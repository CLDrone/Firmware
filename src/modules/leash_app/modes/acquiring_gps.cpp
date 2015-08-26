#include "acquiring_gps.h"

#include <stdio.h>

#include "../displayhelper.h"
#include "../datamanager.h"

#include "main.h"
#include "menu.h"

//TODO[AK] These constants taken from position_estimator_inav, should by parametrized I guess
static const float min_eph_epv = 2.0f;	// min EPH/EPV, used for weight calculation
static const float max_eph_epv = 20.0f;	// max EPH/EPV acceptable for estimation

namespace modes
{

bool drone_status(DataManager *dm);

Acquiring_gps::Acquiring_gps() :
    airdogGPS(NO_GPS),
    leashGPS(NO_GPS),
    drone_has_gps(false),
    drone_has_home(false),
    leash_has_gps(false),
    leash_has_home(false)
{
    checkGPS();
    DisplayHelper::showMain(MAINSCREEN_INFO, "Getting GPS",
                            AIRDOGMODE_NONE, FOLLOW_PATH, LAND_SPOT,
                            leashGPS, airdogGPS);
}

int Acquiring_gps::getTimeout()
{
    return -1;
}

void Acquiring_gps::listenForEvents(bool awaitMask[])
{
    awaitMask[FD_KbdHandler] = 1;
    awaitMask[FD_LocalPos] = 1;
    awaitMask[FD_VehicleStatus] = 1;
    awaitMask[FD_DroneLocalPos] = 1;
}

Base* Acquiring_gps::doEvent(int orbId)
{
    DataManager *dm = DataManager::instance();
    Base *nextMode = nullptr;

    if (orbId == FD_KbdHandler)
    {
        if (key_pressed(BTN_MODE))
        {
            nextMode = new Menu();
        }
    }
    else if (orbId == FD_LocalPos)
    {
        if (dm->localPos.xy_valid)
        {
            leash_has_gps = true;
        }
        checkGPS();
        DisplayHelper::showMain(MAINSCREEN_INFO, "Getting GPS",
                                AIRDOGMODE_NONE, FOLLOW_PATH, LAND_SPOT,
                                leashGPS, airdogGPS);
    }
    else if (orbId == FD_VehicleStatus)
    {
        if (dm->vehicle_status.condition_home_position_valid)
        {
            leash_has_home = true;
        }
    }
    else if (orbId == FD_DroneLocalPos)
    {
        drone_has_gps = drone_status(dm);
        // Since we are subscribing to TargetGlobalPos topic - it is not 0 only if we have home already
        drone_has_home = drone_has_gps;
        checkGPS();
        DisplayHelper::showMain(MAINSCREEN_INFO, "Getting GPS",
                                AIRDOGMODE_NONE, FOLLOW_PATH, LAND_SPOT,
                                leashGPS, airdogGPS);
    }

    if (bothGotGPS())
    {
        nextMode = new Main();
    }
    // Check if we are in service screen
    Base* service = checkServiceScreen(orbId);
    if (service)
        nextMode = service;

    return nextMode;
}
bool Acquiring_gps::bothGotGPS()
{
    return (drone_has_gps && drone_has_home && leash_has_home && leash_has_gps);
}

void Acquiring_gps::checkGPS()
{
    DataManager *dm = DataManager::instance();
    float leash_eph = dm->localPos.eph;
    float airdog_eph = dm->droneLocalPos.eph;

    if (leash_eph == 0.0f)
        leashGPS = NO_GPS;
    else if (leash_eph < 1.5f)
        leashGPS = EXCELENT_GPS;
    else if (leash_eph < 2.5f)
        leashGPS = GOOD_GPS;
    else if (leash_eph < 3.2f)
        leashGPS = FAIR_GPS;
    else
        leashGPS = BAD_GPS;

    if (airdog_eph == 0.0f)
        airdogGPS = NO_GPS;
    else if (airdog_eph < 1.5f)
        airdogGPS = EXCELENT_GPS;
    else if (airdog_eph < 2.5f)
        airdogGPS = GOOD_GPS;
    else if (airdog_eph < 3.2f)
        airdogGPS = FAIR_GPS;
    else
        airdogGPS = BAD_GPS;
}

bool drone_status(DataManager *dm)
{
    bool result = false;
    float eph = dm->droneLocalPos.eph;
    float epv = dm->droneLocalPos.epv;

    // Since we are subscribing to TargetGlobalPos topic - it is not 0 only if we have home already
    if (eph != 0.0f && epv != 0.0f){
        result = true;
    }
    return result;
}

}