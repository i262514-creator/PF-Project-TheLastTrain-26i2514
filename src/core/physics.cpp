#include "physics.h"
#include "util.h"
#include <cstdlib>

using namespace std;
bool hasItem(const char inventory[], int inventoryCount, char itemSymbol){
    if(inventoryCount < 0 || inventoryCount > MAX_INVENTORY_CAPACITY){
        return false;
    }
    for(int i = 0; i < inventoryCount; i++){
        if(inventory[i] == itemSymbol){
            return true;
        }
    }
    return false;
}

bool addItem(char inventory[], int &inventoryCount, char itemSymbol){
    if(inventoryCount < 0 || inventoryCount >= MAX_INVENTORY_CAPACITY){
        return false;
    }
    if(hasItem(inventory, inventoryCount, itemSymbol)){
        return false;
    }
    inventory[inventoryCount] = itemSymbol;
    inventoryCount++;
    return true;
}

bool removeItem(char inventory[], int &inventoryCount, char itemSymbol) {
    if(inventoryCount < 0 || inventoryCount > MAX_INVENTORY_CAPACITY){
        return false;
    }
    for(int i = 0; i < inventoryCount; i++){
        if(inventory[i] == itemSymbol){
            for(int j = i; j < inventoryCount - 1; j++){
                inventory[j] = inventory[j + 1];
            }
            inventoryCount--;
            inventory[inventoryCount] = '\0';
            return true;
        }
    }
    return false;
}

void applyResourceDelta(int &energy, int &time, int deltaEnergy, int deltaTime){
    energy = clampInt(energy + deltaEnergy, 0, MAX_ENERGY);
    time = clampInt(time + deltaTime, 0, MAX_TIME);
}

void applyScoreDelta(int &score, int deltaScore){
    score += deltaScore;
}

bool isDirectlyConnected(int fromF, int fromR, int toF, int toR,
                         const char horizontalConn[NUM_FLOORS][ROOMS_PER_FLOOR - 1],
                         char &outConnType){
    outConnType = CONN_NONE;
    if(fromF < 1 || fromF > NUM_FLOORS || toF < 1 || toF > NUM_FLOORS || fromR < 1 ||
        fromR > ROOMS_PER_FLOOR || toR < 1 || toR > ROOMS_PER_FLOOR){
        return false;
    }
    if(fromF == toF && fromR == toR){
        return false;
    }

    if(fromF == toF && abs(fromR - toR) == 1){
        int leftRoom = fromR;
        if (toR < fromR) {
            leftRoom = toR;
        }
        outConnType = horizontalConn[fromF - 1][leftRoom - 1];
    }

    if(fromR == 1 && toR == 1){
        bool stair1 = (fromF == 1 && toF == 2) || (fromF == 2 && toF == 1);
        bool stair2 = (fromF == 2 && toF == 3) || (fromF == 3 && toF == 2);
        bool stair3 = (fromF == 4 && toF == 5) || (fromF == 5 && toF == 4);
        if(stair1 || stair2 || stair3){
            outConnType = 'S';
        }
    }

    if(fromF != toF && fromF <= 2 && toF <= 2 && fromR >= 2 && fromR <= 3 && toR >= 2 &&
        toR <= 3){
        outConnType = '1';
    }

    bool fromLift2 = fromF == 2 || fromF == 4 || fromF == 5;
    bool toLift2 = toF == 2 || toF == 4 || toF == 5;
    if(fromF != toF && fromR == 4 && toR == 4 && fromLift2 && toLift2){
        outConnType = '2';
    }
    return outConnType != CONN_NONE && outConnType != CONN_SOLID_WALL;
}

bool canTraverse(int targetFloor, int targetRoom, char connType, const char inventory[],
                 int inventoryCount, int emergencyTripsLeft, bool lift2Operational){
    if(targetFloor < 1 || targetFloor > NUM_FLOORS || targetRoom < 1 || targetRoom > ROOMS_PER_FLOOR){
        return false;
    }
    if(connType == CONN_LOCKED_DOOR){
        return hasItem(inventory, inventoryCount, ENT_MASTER_KEY);
    }
    if(connType == '2'){
        if(lift2Operational){
            return hasItem(inventory, inventoryCount, ENT_LIFT_PASS);
        }
        return emergencyTripsLeft > 0 && hasItem(inventory, inventoryCount, ENT_EMERGENCY_CARD);
    }
    return connType == CONN_DOOR || connType == CONN_WINDOW || connType == 'S' || connType == '1';
}
