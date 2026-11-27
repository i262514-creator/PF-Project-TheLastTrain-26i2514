#include "level.h"
#include "physics.h"
#include <cstring>
#include <fstream>
#include <new>
#include <sstream>
#include <string>

using namespace std;

const char *getEntityName(char entitySymbol){
    switch (entitySymbol){
    case ENT_MASTER_KEY:
        return "Master Key";
    case ENT_ENERGY_PACK:
        return "Energy Pack";
    case ENT_EMERGENCY_TOOLKIT:
        return "Emergency Toolkit";
    case ENT_CLUE_4:
        return "Clue 4";
    case ENT_FUSE:
        return "Fuse";
    case ENT_CAT:
        return "Cat";
    case ENT_CONTROL_PANEL:
        return "Control Panel";
    case ENT_LIFT_PASS:
        return "Lift Pass";
    case ENT_PASSENGER:
        return "Injured Passenger";
    case ENT_MEDICAL_KIT:
        return "Medical Kit";
    case ENT_EMERGENCY_CARD:
        return "Emergency Access Card";
    case ENT_CLUE_2:
        return "Clue 2";
    case ENT_CLUE_7:
        return "Clue 7";
    case ENT_CABINET_TRAIN_KEY:
        return "Cabinet / Train Key";
    case ENT_CAT_CARRIER:
        return "Cat Carrier";
    case ENT_OBSERVATION_TOKEN:
        return "Observation Token";
    default:
        return "Unknown";
    }
}


static char readConnection(const string &gap){

    if(gap.find("L1") != string::npos){
        return '1'; }

    char connection = CONN_NONE;
    
    for(size_t i = 0; i < gap.size(); i++){
        char symbol = gap[i];
        
        if (symbol == 'D' || symbol == 'W' || symbol == 'L' || symbol == '#'){
            if (connection != CONN_NONE){
                return CONN_NONE; 
            }
            connection=symbol;
        }
    }
    return connection;
}


bool loadBuildingMap(const char *filename, char horizontalConn[NUM_FLOORS][ROOMS_PER_FLOOR - 1],
                     char roomEntities[NUM_FLOORS][ROOMS_PER_FLOOR][MAX_ENTITIES_PER_ROOM],
                     int entityCounts[NUM_FLOORS][ROOMS_PER_FLOOR]){
    ifstream input(filename);
    if(!input) {
        return false;
    } 

    char newConnections[NUM_FLOORS][ROOMS_PER_FLOOR-1] = {};
    char newEntities[NUM_FLOORS][ROOMS_PER_FLOOR][MAX_ENTITIES_PER_ROOM] = {};
    int newCounts[NUM_FLOORS][ROOMS_PER_FLOOR]={};
    bool floorFound[NUM_FLOORS]={};
    string line;
    string entityLine;

    while(getline(input, line)){
        if(line.find('|') == string::npos){
            continue; 
        }
        size_t firstLabel = line.find('F');
        if(firstLabel == string::npos || firstLabel + 4 >= line.size()){
            continue;
        }
        if(line[firstLabel + 2] != '-' || line[firstLabel + 3] != 'R' || line[firstLabel + 4] != '1'){
            continue;
        }
        int floorIndex = line[firstLabel + 1] - '1';
        if(floorIndex < 0 || floorIndex >= NUM_FLOORS || floorFound[floorIndex]){
            return false;
        }

        size_t columns[ROOMS_PER_FLOOR];
        for(int room = 0; room < ROOMS_PER_FLOOR; room++){
            string label = "F" + to_string(floorIndex + 1) + "-R" + to_string(room + 1);
            columns[room] = line.find(label);
            if(columns[room] == string::npos){
                return false;
            }
            if(room > 0){
                if(columns[room] < columns[room - 1] + 5){
                    return false;
                }
                size_t gapStart = columns[room - 1] + 5;
                string gap = line.substr(gapStart, columns[room] - gapStart);
                char connection = readConnection(gap);
                if(connection == CONN_NONE){
                    return false;
                }
                newConnections[floorIndex][room - 1] = connection;
            }
        }

        if(!getline(input, entityLine)){
            return false;
        }
        for(int room = 0; room < ROOMS_PER_FLOOR; room++){
            size_t begin = columns[room];
            size_t end;
            if(room < ROOMS_PER_FLOOR - 1){
                end = columns[room + 1] - 2;
            }else{
                end = entityLine.find('|', begin);
            }
            if(end == string::npos || end < begin || end > entityLine.size()){
                return false;
            }
            for(size_t column = begin; column < end; column++){
                char symbol = entityLine[column];
                if(symbol == ' ' || symbol == '\r'){
                    continue;
                }
                if(strchr("<)\\4*~;(!$%27^@&", symbol) == nullptr){
                    return false;
                }
                int copies = 1;
                if(symbol == ENT_ENERGY_PACK){
                    copies = 2; 
                }
                for(int copy = 0; copy < copies; copy++){
                    int &count = newCounts[floorIndex][room];
                    if(count >= MAX_ENTITIES_PER_ROOM) {
                        return false;
                    }
                    newEntities[floorIndex][room][count] = symbol;
                    count++;
                }
            }
        }
        floorFound[floorIndex] = true;
    }
    for(int floor = 0; floor < NUM_FLOORS; floor++){
        if(!floorFound[floor]){
            return false;
        }
    }
    if(!validateStartingRoom(1, 1, newConnections)){
        return false;
    }
    for(int floor = 0; floor < NUM_FLOORS; floor++) {
        for(int room = 0; room < ROOMS_PER_FLOOR; room++){
            entityCounts[floor][room] = newCounts[floor][room];
            for(int i = 0; i < MAX_ENTITIES_PER_ROOM; i++) {
                roomEntities[floor][room][i] = newEntities[floor][room][i];
            }
            if(room < ROOMS_PER_FLOOR - 1){
                horizontalConn[floor][room] = newConnections[floor][room];
            }
        }
    }
    return true;
}

bool validateStartingRoom(int startFloor, int startRoom,
                          const char horizontalConn[NUM_FLOORS][ROOMS_PER_FLOOR - 1]) {
    if(startFloor < 1 || startFloor > NUM_FLOORS || startRoom < 1 || startRoom > ROOMS_PER_FLOOR){
        return false;
    }
    int *queue = new (nothrow) int[TOTAL_ROOMS];
    if(queue == nullptr) {
        return false;
    }
    bool visited[TOTAL_ROOMS] = {};
    int head = 0;
    int tail = 0;
    int start = (startFloor - 1) * ROOMS_PER_FLOOR + startRoom - 1;
    queue[tail] = start;
    tail++;
    visited[start] = true;

    while(head < tail){
        int current = queue[head];
        head++;
        for(int next = 0; next < TOTAL_ROOMS; next++){
            int fromFloor = current / ROOMS_PER_FLOOR + 1;
            int fromRoom = current % ROOMS_PER_FLOOR + 1;
            int toFloor = next / ROOMS_PER_FLOOR + 1;
            int toRoom = next % ROOMS_PER_FLOOR + 1;
            char connection;
            if(!visited[next] && isDirectlyConnected(fromFloor, fromRoom, toFloor, toRoom,
                                                      horizontalConn, connection)){
                visited[next] = true;
                queue[tail] = next;
                tail++;
            }
        }
    }
    delete[] queue; 
    return tail == TOTAL_ROOMS;
}

bool loadInitialState(const char *filename, int &playerFloor, int &playerRoom, int &energy,
                      int &time, int &score, char inventory[], int &inventoryCount,
                      bool objectives[], int &emergencyTripsLeft){
    ifstream input(filename);
    if(!input){
        return false;
    }
    int newEnergy = -1;
    int newTime = -1;
    int newScore = -1;
    int newFloor = 1;
    int newRoom = 1;
    bool startFound = false;
    string line;

    while(getline(input, line)){
        if(!line.empty() && line.back() == '\r'){
            line.pop_back();
        }
        size_t colon = line.find(':');
        if(colon == string::npos){
            continue;
        }
        string key = line.substr(0, colon);
        istringstream value(line.substr(colon + 1));
        string extra;
        if(key == "INITIAL_ENERGY" || key == "INITIAL_TIME" || key == "INITIAL_SCORE"){
            int number;
            if(!(value >> number) || (value >> extra)){
                return false;
            }
            if(key == "INITIAL_ENERGY"){
                newEnergy = number;
            }else if (key == "INITIAL_TIME"){
                newTime = number;
            }else{
                newScore = number;
            }
        } else if(key == "START_ROOM"){
            string roomName;
            if(!(value >> roomName) || (value >> extra)){
                return false;
            }
            if(roomName != "PLAYER_SELECTS"){
                if(roomName.size() != 5 || roomName[0] != 'F' || roomName[2] != '-' || roomName[3] != 'R'){
                    return false;
                }
                newFloor = roomName[1] - '0';
                newRoom = roomName[4] - '0';
            }
            startFound = true;
        } else if(key == "INITIAL_INVENTORY" || key == "CLUES_COLLECTED"){
            string setting;
            if(!(value >> setting) || setting != "NONE" || (value >> extra)){
                return false; 
            }
        } else if(key == "PASSENGER_RESCUED" || key == "CAT_RESCUED" ||
                   key == "CABINET_UNLOCKED" || key == "TRAIN_KEY_COLLECTED" ||
                   key == "FUSE_INSTALLED" || key == "LIFT_PASS_COLLECTED" ||
                   key == "TRAIN_PREPARED" || key == "PLAYER_AT_TRAIN"){
            string setting;
            if(!(value >> setting) || setting != "NO" || (value >> extra)){
                return false;
            }
        }
    }
    if(!startFound || newEnergy < 1 || newEnergy > MAX_ENERGY || newTime < 1 ||
        newTime > MAX_TIME || newScore != INITIAL_SCORE || newFloor < 1 || newFloor > NUM_FLOORS ||
        newRoom < 1 || newRoom > ROOMS_PER_FLOOR){
        return false;
    }
    playerFloor = newFloor;
    playerRoom = newRoom;
    energy = newEnergy;
    time = newTime;
    score = newScore;
    inventoryCount = 0;
    emergencyTripsLeft = 0;
    for(int i = 0; i < MAX_INVENTORY_CAPACITY; i++){
        inventory[i] = '\0';
    }
    for(int i = 0; i < NUM_OBJECTIVES; i++){
        objectives[i] = false;
    }
    return true;
}