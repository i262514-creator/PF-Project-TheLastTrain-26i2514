#include "logger.h"
#include "physics.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

using namespace std;

static string sessionAudit;

void resetSessionAudit(){
    sessionAudit = "";
}

void appendSessionAudit(const char *command, int floor, int room, int energy, int time, int score){
    ostringstream line;
    line<<command<< " | F"<<floor<<"-R"<<room <<" E="<< energy<< " T="<<time<<" S="<<score<<'\n';
    sessionAudit += line.str();
}

static int scoreFromObjectives(const bool objectives[]){
    const int awards[NUM_OBJECTIVES] = {SCORE_KEY_COLLECT, SCORE_CLUE_FOUND, SCORE_CLUE_FOUND, SCORE_CLUE_FOUND,
                                        SCORE_CABINET_UNLOCK, SCORE_TRAIN_KEY, 0, SCORE_INSTALL_FUSE, SCORE_LIFT_PASS,
                                        0, SCORE_RESCUE_PASSENGER, 0, SCORE_SECURE_CAT, SCORE_PREPARE_TRAIN,
                                        SCORE_MISSION_VICTORY, 0};
    int score = 0;
    for(int i = 0; i < NUM_OBJECTIVES; i++){
        if(objectives[i]){
            score += awards[i];
        }
    }
    return score;
}

static bool validSavedObjectives(const bool objectives[], const char inventory[], int inventoryCount, int energy, 
                                 int time, int score, int emergencyTripsLeft){

    if(score != scoreFromObjectives(objectives)){
        return false;
    }
    if(objectives[OBJ_FUSE_INSTALLED] != objectives[OBJ_LIFT2_ACTIVE]){
        return false;
    }
    if(objectives[OBJ_FUSE_INSTALLED] && !objectives[OBJ_FUSE_COLLECTED]){
        return false;
    }
    if(objectives[OBJ_CABINET_UNLOCKED] &&
        (!objectives[OBJ_CLUE_4] || !objectives[OBJ_CLUE_2] || !objectives[OBJ_CLUE_7])){
        return false;
    }
    if(objectives[OBJ_TRAIN_KEY] && !objectives[OBJ_CABINET_UNLOCKED]){
        return false;
    }
    if(objectives[OBJ_CAT_SECURED] && !objectives[OBJ_CAT_CARRIER]){
        return false;
    }
    if(objectives[OBJ_PASSENGER_RESCUED] && !hasItem(inventory, inventoryCount, ENT_MEDICAL_KIT)){
        return false;
    }
    if(objectives[OBJ_MISSION_FAILED] != (energy == 0 || time == 0)){
        return false;
    }
    if(objectives[OBJ_TRAIN_PREPARED] != objectives[OBJ_MISSION_WON]){
        return false;
    }
    if(objectives[OBJ_MISSION_WON]) {
        if(objectives[OBJ_MISSION_FAILED] || !objectives[OBJ_PASSENGER_RESCUED] ||
            !objectives[OBJ_CAT_SECURED] || !objectives[OBJ_TRAIN_KEY] ||
            !objectives[OBJ_FUSE_INSTALLED] || !objectives[OBJ_LIFT_PASS]){
            return false;
        }
    }
    if(hasItem(inventory, inventoryCount, ENT_MASTER_KEY) != objectives[OBJ_MASTER_KEY] ||
        hasItem(inventory, inventoryCount, ENT_LIFT_PASS) != objectives[OBJ_LIFT_PASS] ||
        hasItem(inventory, inventoryCount, ENT_CABINET_TRAIN_KEY) != objectives[OBJ_TRAIN_KEY] ||
        hasItem(inventory, inventoryCount, ENT_CAT_CARRIER) != objectives[OBJ_CAT_CARRIER]){
        return false;
    }
    bool shouldCarryFuse = objectives[OBJ_FUSE_COLLECTED] && !objectives[OBJ_FUSE_INSTALLED];
    if(hasItem(inventory, inventoryCount, ENT_FUSE) != shouldCarryFuse){
        return false;
    }
    if(emergencyTripsLeft > 0 && !hasItem(inventory, inventoryCount, ENT_EMERGENCY_CARD)){
        return false;
    }
    return true;
}

bool saveGameStateToFile(
    const char *filename, int playerFloor, int playerRoom, int energy, int time, int score,
    const char inventory[], int inventoryCount, const bool objectives[], int emergencyTripsLeft,
    const char roomEntities[NUM_FLOORS][ROOMS_PER_FLOOR][MAX_ENTITIES_PER_ROOM],
    const int entityCounts[NUM_FLOORS][ROOMS_PER_FLOOR]){
    
    ofstream output(filename);
    if(!output){
        return false;
    }
    output<<"LAST_TRAIN_SAVE 2\n";
    output<<playerFloor<<' '<<playerRoom<<' '<<energy<<' '<<time<<' '<<score<<' '<<emergencyTripsLeft<<'\n';
    output<<inventoryCount;
    for(int i = 0; i < inventoryCount; i++){
        output<<' '<<static_cast<int>(inventory[i]);
    }
    output<<'\n';
    for(int i = 0; i < NUM_OBJECTIVES; i++){
        output<<objectives[i]<<' ';
    }
    output<<'\n';
    for(int floor = 0; floor < NUM_FLOORS; floor++){
        for(int room = 0; room < ROOMS_PER_FLOOR; room++){
            output<<entityCounts[floor][room];
            for(int i = 0; i < entityCounts[floor][room]; i++){
                output<<' '<< static_cast<int>(roomEntities[floor][room][i]);
            }
            output << '\n';
        }
    }
    output<<"AUDIT\n"<<sessionAudit<<"END\n";
    output.close();
    return static_cast<bool>(output);
}

bool loadGameStateFromFile(const char *filename, int &playerFloor, int &playerRoom, int &energy,
                           int &time, int &score, char inventory[], int &inventoryCount,
                           bool objectives[], int &emergencyTripsLeft,
                           char roomEntities[NUM_FLOORS][ROOMS_PER_FLOOR][MAX_ENTITIES_PER_ROOM],
                           int entityCounts[NUM_FLOORS][ROOMS_PER_FLOOR]){
    ifstream input(filename);
    if(!input){
        return false;
    }

    int newFloor, newRoom, newEnergy, newTime, newScore, newTrips, newInventoryCount;
    char newInventory[MAX_INVENTORY_CAPACITY] = {};
    bool newObjectives[NUM_OBJECTIVES] = {};
    char newEntities[NUM_FLOORS][ROOMS_PER_FLOOR][MAX_ENTITIES_PER_ROOM] = {};
    int newCounts[NUM_FLOORS][ROOMS_PER_FLOOR] = {};
    string magic;
    int version;
    if(!(input >> magic >> version) || magic != "LAST_TRAIN_SAVE" || version != 2) {
        return false;
    }
    if(!(input >> newFloor >> newRoom >> newEnergy >> newTime >> newScore >> newTrips)) {
        return false;
    }
    if(newFloor < 1 || newFloor > NUM_FLOORS || newRoom < 1 || newRoom > ROOMS_PER_FLOOR ||
        newEnergy < 0 || newEnergy > MAX_ENERGY || newTime < 0 || newTime > MAX_TIME ||
        newScore < 0 || newScore > 520 || newTrips < 0 || newTrips > 3) {
        return false;
    }
    if(!(input >> newInventoryCount) || newInventoryCount < 0 || newInventoryCount > MAX_INVENTORY_CAPACITY){
        return false;
    }
    for(int i = 0; i < newInventoryCount; i++){
        int code;
        if(!(input >> code) || code <= 0 || code > 127){
            return false;
        }
        char symbol = static_cast<char>(code);
        if(string("<*(%$^@").find(symbol) == string::npos || hasItem(newInventory, i, symbol)){
            return false;
        }
        newInventory[i] = symbol;
    }
    for(int i = 0; i < NUM_OBJECTIVES; i++){
        int flag;
        if(!(input >> flag) || (flag != 0 && flag != 1)){
            return false;
        }
        newObjectives[i] = flag == 1;
    }
    int frequencies[128] = {};
    for(int floor = 0; floor < NUM_FLOORS; floor++){
        for(int room = 0; room < ROOMS_PER_FLOOR; room++){
            int &count = newCounts[floor][room];
            if(!(input >> count) || count < 0 || count > MAX_ENTITIES_PER_ROOM){
                return false;
            }
            for(int i = 0; i < count; i++){
                int code;
                if(!(input >> code) || code <= 0 || code > 127){
                    return false;
                }
                char symbol = static_cast<char>(code);
                if(string("<)\\4*~;(!$%27^@&").find(symbol) == string::npos){
                    return false;
                }
                frequencies[code]++;
                int maximumCopies = 1;
                if(symbol == ENT_ENERGY_PACK){
                    maximumCopies = 2;
                }
                if(frequencies[code] > maximumCopies){
                    return false;
                }
                newEntities[floor][room][i] = symbol;
            }
        }
    }
    string marker;
    if(!(input >> marker) || marker != "AUDIT"){
        return false;
    }
    string line;
    getline(input, line); 
    if(line != "" && line != "\r"){
        return false;
    }
    string newAudit;
    bool endFound = false;
    int auditLines = 0;
    while(getline(input, line)){
        if(!line.empty() && line.back() == '\r'){
            line.pop_back();
        }
        if(line == "END"){
            endFound = true;
            break;
        }
        auditLines++;
        if(line.empty() || line.size() > MAX_LINE_LEN || auditLines > 1000){
            return false;
        }
        newAudit += line + '\n';
    }
    string extra;
    if(!endFound || (input >> extra)){
        return false;
    }
    if(!validSavedObjectives(newObjectives, newInventory, newInventoryCount, newEnergy, newTime, newScore, newTrips)){
        return false;
    }
    if(newObjectives[OBJ_MISSION_WON] && (newFloor != 5 || newRoom != 2)){
        return false;
    }

    playerFloor = newFloor;
    playerRoom = newRoom;
    energy = newEnergy;
    time = newTime;
    score = newScore;
    emergencyTripsLeft = newTrips;
    inventoryCount = newInventoryCount;
    for(int i = 0; i < MAX_INVENTORY_CAPACITY; i++){
        inventory[i] = newInventory[i];
    }
    for(int i = 0; i < NUM_OBJECTIVES; i++){
        objectives[i] = newObjectives[i];
    }
    for(int floor = 0; floor < NUM_FLOORS; floor++){
        for(int room = 0; room < ROOMS_PER_FLOOR; room++){
            entityCounts[floor][room] = newCounts[floor][room];
            for(int i = 0; i < MAX_ENTITIES_PER_ROOM; i++){
                roomEntities[floor][room][i] = newEntities[floor][room][i];
            }
        }
    }
    sessionAudit = newAudit;
    return true;
}

void writeGameSummaryReport(const char *filename, int score, int remainingEnergy, int remainingTime,
                            const bool objectives[], bool victory) {
    ofstream output(filename);
    if(!output){
        cerr << "Error: could not write final report.\n";
        return;
    }
    output<< "THE LAST TRAIN - FINAL REPORT\n";
    if(victory){
        output<< "Result: VICTORY\n";
    }else{
        output<< "Result: DEFEAT\n";
    }
    output<<"Score: "<<score<< " / 520\nEnergy: " << remainingEnergy << "\nTime: " << remainingTime << "\n\nOBJECTIVES\n";
    const char *names[NUM_OBJECTIVES] = {"Master Key", "Clue 4", "Clue 2", "Clue 7", "Cabinet unlocked", "Train Key",
                                        "Fuse collected", "Fuse installed", "Lift Pass", "Lift 2 active", "Passenger rescued",
                                        "Cat Carrier", "Cat secured", "Train prepared and boarded", "Mission won", "Mission failed"};

    for(int i = 0; i < NUM_OBJECTIVES; i++){
        if(objectives[i]){
            output << "[x] ";
        }else {
            output << "[ ] ";
        }
        output<<names[i] << '\n';
    }
    output << "\nSESSION AUDIT (accepted actions, post-action resources)\n";
    output << sessionAudit;
}

