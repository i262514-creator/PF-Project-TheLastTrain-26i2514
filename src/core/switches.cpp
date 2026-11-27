#include "switches.h"
#include "physics.h"
#include "util.h"
#include <cstddef>

using namespace std;

bool canUseLift1(int fromFloor, int toFloor){
    return fromFloor != toFloor && fromFloor >= 1 && fromFloor <= 2 && toFloor >= 1 && toFloor <= 2;
}

bool canUseLift2(int fromFloor, int toFloor, bool lift2Operational, int emergencyTripsLeft){
    bool validFrom = fromFloor == 2 || fromFloor == 4 || fromFloor == 5;
    bool validTo = toFloor == 2 || toFloor == 4 || toFloor == 5;
    return fromFloor != toFloor && validFrom && validTo &&
           (lift2Operational || emergencyTripsLeft > 0);
}

bool unlockFloor3Door(const char inventory[], int inventoryCount){
    return hasItem(inventory, inventoryCount, ENT_MASTER_KEY);
}

bool installFuseControlPanel(char inventory[], int &inventoryCount, bool objectives[], int &score){
    if(objectives[OBJ_FUSE_INSTALLED]){
        return false;
    }
    if(!removeItem(inventory, inventoryCount, ENT_FUSE)){
        return false;
    }
    objectives[OBJ_FUSE_INSTALLED] = true;
    objectives[OBJ_LIFT2_ACTIVE] = true;
    applyScoreDelta(score, SCORE_INSTALL_FUSE);
    return true;
}

bool verifyCabinetCode(const char *enteredCode, const bool objectives[]){
    if(!objectives[OBJ_CLUE_4] || !objectives[OBJ_CLUE_2] || !objectives[OBJ_CLUE_7]){
        return false;
    }
    return stringEquals(enteredCode, "427");
}
