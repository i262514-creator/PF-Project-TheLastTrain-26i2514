#include "viewer_iface.h"
#include "util.h"
#include "level.h"
#include <fstream>
#include <iostream>
#include <string>

using namespace std;

void renderDashboard(int playerFloor, int playerRoom, int energy, int time, int score,
                     const char inventory[], int inventoryCount, int emergencyTripsLeft){

    cout<< "\n[ F" << playerFloor << "-R" << playerRoom << " | Energy " << energy << '/'
         << MAX_ENERGY << " | Time " << time << '/' << MAX_TIME << " | Score " << score << "/520"
         << " | Emergency trips " << emergencyTripsLeft << " ]\n";
    cout<< "Inventory:";

    if(inventoryCount == 0){
        cout<< " (empty)";
    }
    for (int i = 0; i < inventoryCount; i++) {
        cout<< " [" << inventory[i] << "] " << getEntityName(inventory[i]);
    }
    cout<< '\n';
}

void renderRoomDetails(int floor, int room,
                       const char roomEntities[NUM_FLOORS][ROOMS_PER_FLOOR][MAX_ENTITIES_PER_ROOM],
                       int entityCount){
    if(floor < 1 || floor > NUM_FLOORS || room < 1 || room > ROOMS_PER_FLOOR || entityCount < 0 ||
        entityCount > MAX_ENTITIES_PER_ROOM) {
        return;
    }
    cout<< "Room F" << floor << "-R" << room << ":\n";
    for (int i = 0; i < entityCount; i++){
        char symbol = roomEntities[floor - 1][room - 1][i];
        cout << "  [" << symbol << "] " << getEntityName(symbol) << '\n';
    }
    if(entityCount == 0){
        cout<< "  No remaining items.\n";
    }
}

void renderAsciiBuilding(int playerFloor, int playerRoom){
    printHeader("BUILDING MAP");
    ifstream mapFile("data/building.txt");
    if(!mapFile){
        cout << "Could not display data/building.txt\n";
        return;
    }
    string line;
    while(getline(mapFile, line)){
        if(!line.empty() && line.back() == '\r'){
            line.pop_back();
        }
        cout<< line << '\n';
    }
    cout<< "\nYOU ARE AT F" << playerFloor << "-R" << playerRoom << '\n';
    cout<< "Map shows original item locations; inspect shows remaining room items.\n";
}

void renderObjectivesChecklist(const bool objectives[]) {
    const int indices[] = {
        OBJ_MASTER_KEY,        OBJ_CLUE_4,      OBJ_CLUE_2,         OBJ_CLUE_7,
        OBJ_PASSENGER_RESCUED, OBJ_CAT_SECURED, OBJ_FUSE_INSTALLED, OBJ_LIFT_PASS,
        OBJ_CABINET_UNLOCKED,  OBJ_TRAIN_KEY,   OBJ_TRAIN_PREPARED};
    const char *names[] = {"Master Key collected",
                           "Clue 4 found",
                           "Clue 2 found",
                           "Clue 7 found",
                           "Passenger rescued",
                           "Cat secured",
                           "Fuse installed",
                           "Lift Pass collected",
                           "Cabinet unlocked",
                           "Train Key collected",
                           "Train prepared and boarded"};
    printHeader("OBJECTIVES");
    for(int i = 0; i < 11; i++){
        if(objectives[indices[i]]){
            cout << "[x] ";
        }else {
            cout << "[ ] ";
        }
        cout<< names[i] << '\n';
    }
}
