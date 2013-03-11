/*!
 * \file tfrdump.cpp
 * \author Mark Morschhäuser
 * \date 2013-02-09
 * \brief Program to read savegamefiles (*.TFR) from Lucas Arts TIE FIGHTER and dump the contents.
 *
 * \mainpage
 * \section Description
 * This program opens savegamefiles (*.TFR) from Lucas Arts TIE FIGHTER, reads and interprets known byte offsets, then prints
 * everything known so far.
 * The code was tested with the german TIE Fighter CD-ROM Edition and works on Linux AMD64 systems, on Big Endian systems the betoW/betoDW 
 * functions should not be used because they change the endianness of larger datatypes, but as I don't use Windows I did not write some 
 * OS-aware routines regarding the endianness.
 * 
 * It is not complicated to extend the code to also <em>write</em> values into TFR files but it is not yet possible to use this code to
 * completely create such a file because there are some offset ranges with unknown purpose, e.g. the first two bytes of each file.
 * 
 * I'd like to have a class or struct which maps the TFR content exactly, thus can be written to a TFR file directly, patches welcome :-)
 * 
 * About the TFR format:
 * 1) MS-DOS is a Big Endian system (so is the savegame file), Linux amd64 is Little Endian, so if you use your Linux hexeditor to 
 *    reverse engineer some more, keep in mind that longer datatypes are saved backwards. On Windows it should be fine.
 * 2) Size of the files is always 3855 bytes. If you create a new pilot ingame without entering the lobby, all bytes are zero.
 *    After entering the lobby some initial values are set to default values.
 * 3) The file seems to have byteranges which are unused, e.g. there is space for additional missions or because not all battles have the
 *    same amount of missions. Some entries are unused by the game, e.g. the killcounter has a field for super star destroyers and stuff
 *    which never made it into the final game.
 * 4) There are no strings in the file, everything is encoded by integer datatypes using 1 Byte, 2 Bytes, 4 Bytes of size:
 *    BYTE = 1 Byte usable to encode characters, booleans or numbers between 0 - 255
 *    WORD = 2 Bytes to encode numbers from 0 - 65535
 *    DWORD = 4 Bytes to encode numbers from 0 - 4294967295
 * 5) The game keeps it's strings in the file STRINGS.DAT or in LFD ressource files. I did not make the efford to read these, instead
 *    I hardcoded some needed strings like ranks or shipnames etc. Some shipnames are from the german version, I don't have the english
 *    version of the game.
 * 6) I don't list all known offsets here, look at the the member variables and their comments in the Pilot class.
 *    Some entries seem to be repeated at a later position in the file like the pilot status (captured, alive)
 *
 * \section Coding-Remarks
 * I only use C++ STL so no external libraries are needed.
 * I use some C++11 language features though (like the array datatype) so you should use a relatively modern compiler.
 * The Code is kept simple: one class with some helper methods for endianess- or string-translation.
 *
 * Code beautification: \code astyle -A4 tfrdump.cpp \endcode
 *
 * \section Compiling
 * Just invoke a C++11 compatible compiler:
 * \code g++ -std=c++0x tfrdump.cpp -O3 -s -o tfrdump \endcode
 * or
 * \code clang++ -std=c++11 tfrdump.cpp -O3 -s -o tfrdump \endcode
 * 
 * \section Usage
 * \code
 * tfrdump <TFR-File>
 * \endcode
 */

#include <iostream>
#include <fstream>
#include <array>
#include <string>
#include <vector>
#include <cstdlib>
using namespace std;

// Define some DOS/hexedit compatible datatypes that are easy to remember.
typedef uint8_t BYTE;	// 1 Byte: 0 - 255
typedef uint16_t WORD;	// 2 Bytes: 0 - 65535
typedef uint32_t DWORD;	// 4 Bytes: 0 - 4294967295

class Pilot {
private:
    array<BYTE,3855> pilotfilebuffer; // filesize is always 3855 BYTEs and all bytes are x00 for new pilots. Use this array to buffer the file.

    BYTE unused1;		// 00, (always 00? why? purpose?)
    BYTE unused2;		// 01, (always 00? why? purpose?)
    BYTE navyrank;		// 02, (value 00 - 05: { CADET, OFFICER, LIEUTENANT, CAPTAIN, COMMANDER, GENERAL })
    BYTE difficulty;		// 03, (value 00 easy, 01 medium, 02 hard)
    DWORD points;		// 07 06 05 04, (value 00 00 00 00 - ff ff ff ff max works in registry, but overflows ingame; xx xx xx 79 works)
    WORD level;			// 09 08, (value 00 00 - ff ff)
    BYTE secretrank;		// 10, (value 00 - 09: { FIRST_INITIATE, SECOND_CIRCLE, THIRD_CIRCLE, FOURTH_CIRCLE, INNER_CIRCLE, EMPERORS_HAND, EMPERORS_EYES, EMPERORS_VOICE, EMPERORS_REACH})

    /**********************************
    Training certificate: default value: 02, every ship has 3 missions, so value 04 means completed, resulting in a certificate
    **********************************/
    BYTE tf_cert;		// 90
    BYTE ti_cert;		// 91
    BYTE tb_cert;		// 92
    BYTE ta_cert;		// 93
    BYTE gun_cert;		// 94
    BYTE td_cert;		// 95
    BYTE missileboat_cert;	// 96
    BYTE unused_cert[5];	// 97 - 101	(value: 02)

    // If you change Offset 268H to 09 you can choose all battles.
    
    /**********************************
    Medals for Fightsimulation. Every ship has 4 missions, default value: 00, completed: 01. With 2 completed you gain bronze, then silver, then gold medals.
    **********************************/
    BYTE tf_sim[3];		// 520-523
    BYTE ti_sim[3];		// 528-531
    BYTE tb_sim[3];		// 536-539
    BYTE ta_sim[3];		// 544-547
    BYTE gun_sim[3];		// 552-555
    BYTE td_sim[3];		// 560-563
    BYTE missileboat_sim[3];	// 568-571

    /**********************************
     * Active Battle and Missionstatus
     **********************************/
    BYTE activebattle;			// 616		(value: 00-xx? aka the last completed battle)
    array<BYTE,13> battlestatus;	// 617-623	(value: active: 01, killed/captured: 02, complete: 03, killed/captured: 04)
    array<BYTE,13> missionchoose;	// 637-643	(value: 00-06, stays at the max mission # for the battle)

//captured	1628
    /**********************************
    Kills. This is no map container because I need the structure ordered according to the TFR file.
    **********************************/
    array<WORD,68> kills;
    const array<string,68> shipnames = {
        "X-W", //#0 Rebel fighters	@1632
        "Y-W",
        "A-W",
        "B-W",
        "T/F", // Imperial Fighters
        "T/I", //#5 TIE Interceptor
        "T/B",
        "T/A",
        "T/D", // TIE Defender
        "TIE Neu1", //cancelled ship?
        "TIE Neu2", //10 cancelled ship?
        "RAK", // Missile Boat
        "T-W", // T-Wing
        "Z-95",
        "R-41",
        "GUN", //#15 Assault Gunboat
        "FHR", // Tyderian Shuttle
        "E/F", // Escort Shuttle
        "PSC", //Patroullienschiff - System Patrol Craft?
        "SCT", //Scoutship
        "TRN", //#20  Transport
        "ATR", // Assault Transport
        "ETR", // Escort Transport
        "TUG", //Schlepper/Tug
        "MKS", //Mehrzweckkampfschiff/Multipurposebattleship??
        "CN/A", //#25 Container A-D
        "CN/B",
        "CN/C",
        "CN/D",
        "SSL", //Heavy Lifter
        "Heavy Freighter", //#30
        "FRT", //Freighter
        "FFR", //Frachtfähre
        "MTRN",  //Modular Transport
        "CTRN", // Container Transport
        "Neuer Frachter 3", //#35 cancelled ship?
        "MUTR", //Muurian Transport
        "CORT", //Corellian Transport
        "Millenium", //Millenium Falcon? Cancelled :-(
        "KRV", // Korvette
        "M/KRV", //#40 Mod. Korvette
        "FRG", //Nebulon-B Frig.
        "M/FRG", //Mod. Frig.
        "LINER", //C-3 Passagierschiff
        "CRKK", //Carrack Cruiser
        "ANGRK", //#45 Angriffskreuzer
        "EST", // Escort Carrier
        "DREAD", // Dreadnaught
        "LCAL", //Light Calamari
        "AKR", //Abfangkreuzer
        "VSZ", //50 Stardestroyer Victory Class
        "ISZ", // Stardestroyer
        "Super Destroyer", // WHY DID YOU CANCEL THAT??
        "CN/E",
        "CN/F",
        "CN/G", //#55
        "CN/H",
        "CN/I",
        "PLT/1", // platforms/space stations
        "PLT/2",
        "PLT/3", //#60
        "PLT/4",
        "PLT/5",
        "PLT/6",
        "Raumstation7",
        "Raumstation8", //#65
        "Raumstation9",
        "FAB/1" // X/7 Fabrik
    };
    /**********************************/
    DWORD laserhits;		//Laser hits:		1915? 1914? 1913 1912
    DWORD lasersfired;		//Lasers fired:	1911? 1910? 1909 1908
    WORD warheadsfired;		//Fired warheads:	1921 1920
    WORD warheadhits;		//Warhead hits:		1923 1922
    /**********************************
    Rank?			1931 1930 = 1 (Cadet) to 261 (General)
    **********************************
    Trainingsmission Points: 7 ships, 4 missions each - what about completed battles? They can be also played as training.
    **********************************
    TIE Training M1 points	2065 2064
    */
    array<DWORD,28> trainingpoints;

    /**********************************
    Battle Points, there are 77 real missions are in the game, including both addons:
    B1: 6, B2: 5, B3: 6, B4: 5, B5: 5, B6: 4, B7: 5, B8: 6, B9: 6, B10: 6, B11: 7, B12: 7, B13: 8.
    There is place for 8 missions per battle and there are 13 battles -> 104 entries
    **********************************/
    array<DWORD,104> battlepoints;	//Start @2914

    /*********************************
    Stats
    **********************************/
    WORD total;		//3555 3554
    WORD captured;	//3556 BYTE sure, WORD guess
    WORD lost;		//3854

    /*********************/

public:
    Pilot(string);
    friend ostream& operator<<(ostream&, Pilot);
    WORD betoW(unsigned short);
    DWORD betoDW(unsigned short);

    string navyrank_toString();
    string difficulty_toString();
    string secretrank_toString();
    string getmedal(BYTE);
};

/**
 * Translate the current rank number into a string
 */
string Pilot::navyrank_toString()
{
    const string ranks[] = {"Cadet", "Officer", "Lieutenant", "Captain", "Commander", "General"};
    return ranks[navyrank];
}

/**
 * Translate the game difficulty into a string
 */
string Pilot::difficulty_toString()
{
    const string diffs[] = {"easy", "medium", "hard"};
    return diffs[difficulty];
}

/**
 * Translate the current rank of the secret order number into a string
 */
string Pilot::secretrank_toString()
{
    const string ranks[] = {"None", "First Initiate", "Second Circle", "Third Circle", "Fourth Circle",
                            "Inner Circle", "Emperor's Hand", "Emperor's Eyes", "Emperor's Voice",
                            "Emperor's Reach"
                           };
    return ranks[secretrank];
}

/**
 * betoW = Big Endian to WORD.
 * Some Big to lower Endian shifting-magic for WORD data at a given offset:
 * Set x = 0, then add all 1-bits from the second BYTE from offset and shift them 8 positions from right to left.
 * Then add all 1-bits from the first BYE from offset to x.
 * x is now the WORD value from offset but in reversed (aka low endian) order.
 * This is not needed on big endian systems like DOS/Windows; if you use such systems, just append the offset and it's 
 * successor to x without any shifting, patches welcome.
 */
WORD Pilot::betoW(unsigned short offset)
{
    WORD x = 0;
    x |= (BYTE) pilotfilebuffer[offset+1] << 8;
    x |= (BYTE) pilotfilebuffer[offset];
    return x;
}

/**
 * betoDW = Big Endian to DWORD.
 * Some Big to lower Endian shifting-magic for DWORD data at a given offset:
 * Set x = 0, then add all 1-bits from the fourth BYTE from offset and shift them 3*8 positions from right to left.
 * Do the same with the second BYTE but shift by 2*8 positions, then do the same with the first byte shifted by 1*8 positions.
 * x is now the DWORD value from offset but in reversed (aka low endian) order.
 * This is not needed on big endian systems like DOS/Windows; if you use such systems, just append the offset and it's three
 * successors to x without any shifting, patches welcome.
 */
DWORD Pilot::betoDW(unsigned short offset)
{
    DWORD x = 0;
    x |= (BYTE) pilotfilebuffer[offset+3] << 24;
    x |= (BYTE) pilotfilebuffer[offset+2] << 16;
    x |= (BYTE) pilotfilebuffer[offset+1] << 8;
    x |= (BYTE) pilotfilebuffer[offset];
    return x;
}

string Pilot::getmedal(BYTE ship)
{
    switch(ship) {
    case 2:
        return "bronze";
    case 3:
        return "silver";
    case 4:
        return "gold";
    default:
        return "(none)";
    }
}

Pilot::Pilot(string filename)
{
    // create a zero-filled filebuffer, then read the file into it
    pilotfilebuffer.fill(0x0);
    char *readbuffer = new char;
    ifstream pilotstream(filename.c_str(), ios::in|ios::binary);
    if(pilotstream.is_open()) {
        for(int i=0; i < 3855 ; ++i) {
            pilotstream.read(readbuffer,1);
            pilotfilebuffer[i] = (BYTE) *readbuffer;
        }
    }
    pilotstream.close();
    
    // assign values from the buffer to member variables, convert WORD and DWORD values to lower endian.
    navyrank = pilotfilebuffer[2];
    difficulty = pilotfilebuffer[3];	// 03, (value 00 easy, 01 medium, 02 hard)
    points = betoDW(4);			// 07 06 05 04, (value 00 00 00 00 - ff ff ff ff max works in registry, but overflows ingame; xx xx xx 79 works)
    level = betoW(8);			// 09 08, (value 00 00 - ff ff)
    secretrank = pilotfilebuffer[10];

    tf_cert = pilotfilebuffer[90];
    ti_cert = pilotfilebuffer[91];
    tb_cert = pilotfilebuffer[92];
    ta_cert = pilotfilebuffer[93];
    gun_cert = pilotfilebuffer[94];
    td_cert = pilotfilebuffer[95];
    missileboat_cert = pilotfilebuffer[96];
    for(int i=0; i<5; ++i)
        unused_cert[i] = pilotfilebuffer[97+i];	// 97 - 101	(value: 02)

    //Medals for Fightsimulation. There is a DWORD unused space after each ship:
    //Maybe all Battles (even trainings) have place for 8 missions; here we use only 4.
    for(int i=0; i<4; ++i) {
        tf_sim[i] = pilotfilebuffer[520+i];	// 520-523
        ti_sim[i] = pilotfilebuffer[528+i];	// 528-531
        tb_sim[i] = pilotfilebuffer[536+i];	// 536-539
        ta_sim[i] = pilotfilebuffer[544+i];	// 544-547
        gun_sim[i] = pilotfilebuffer[552+i];	// 552-555
        td_sim[i] = pilotfilebuffer[560+i];	// 560-563
        missileboat_sim[i] = pilotfilebuffer[568+i];// 568-571
    }

    activebattle = pilotfilebuffer[616];	// 616		(value: 00-xx? aka the last completed battle)

    for(int i=0; i<battlestatus.size(); i++)
        battlestatus[i] = pilotfilebuffer[617+i];	// 617-623	(value: active: 01, killed/captured: 02, complete: 03, killed/captured: 04)

    for(int i=0; i<missionchoose.size(); i++)
        missionchoose[i] = pilotfilebuffer[637+i];// 637-643	(value: 00-06, stays at the max mission # for the battle)

    for(int i=0; i<kills.size(); i++)
        kills[i] = betoW(1632+i*sizeof(WORD));

    lasersfired = betoDW(1908); //Lasers fired:	1911? 1910? 1909 1908
    laserhits = betoDW(1912); //Laser hits:		1915? 1914? 1913 1912
    warheadsfired = betoW(1920); //Fired warheads:	1921 1920
    warheadhits = betoW(1922); //Warhead hits:		1923 1922

    for(int i=0; i<trainingpoints.size(); i++)
        trainingpoints[i] = betoDW(2064+i*sizeof(DWORD));

    for(int i=0; i<battlepoints.size(); i++)
        battlepoints[i] = betoDW(2914+i*sizeof(DWORD));

    total = betoW(3554);
    captured = betoW(3556);	//3556 BYTE sure, WORD guessed
    lost = betoW(3854);
}

/**
 * Print everything to stdout. Could be adapted to xml or whatever if you plan to write a remake :-)
 */
ostream& operator<<(ostream& out, Pilot p)
{
    out
            << "Navyrank:\t" << p.navyrank_toString() // { CADET, OFFICER, LIEUTENANT, CAPTAIN, COMMANDER, GENERAL };	// 02, (value 00 - 05)
            << endl << "Secret order:\t" << p.secretrank_toString()
            << endl << "Difficulty:\t" << p.difficulty_toString()	// 03, (value 00 easy, 01 medium, 02 hard)
            << endl << "Points:\t\t" << (int)p.points	// 07 06 05 04, (value 00 00 00 00 - ff ff ff ff max works in registry, but overflows ingame; xx xx xx 79 works)
            << endl << "Level:\t\t" << (int)p.level	// 09 08, (value 00 00 - ff ff)
            << endl << "Training Certificates:";
    int certs = 0;
    if((int)p.tf_cert == 0x4) {
        out << " T/F";
        ++certs;
    }
    if((int)p.ti_cert == 0x4) {
        out << " T/I";
        ++certs;
    }
    if((int)p.tb_cert == 0x4) {
        out << " T/B";
        ++certs;
    }
    if((int)p.ta_cert == 0x4) {
        out << " T/A";
        ++certs;
    }
    if((int)p.gun_cert == 0x4) {
        out << " Gunboat";
        ++certs;
    }
    if((int)p.td_cert == 0x4) {
        out << " T/D";
        ++certs;
    }
    if((int)p.missileboat_cert == 0x4) {
        out << " Missile Boat";
        ++certs;
    }
    if(!certs)
        out << " (none)";

    // Medals for Fightsimulation.
    BYTE tf_medal = 0;
    BYTE ti_medal = 0;
    BYTE tb_medal = 0;
    BYTE ta_medal = 0;
    BYTE gun_medal = 0;
    BYTE td_medal = 0;
    BYTE missileboat_medal = 0;

    out << endl << "Ship Medals:";
    for(int i=0; i<4; ++i) {
        tf_medal += p.tf_sim[i];
        ti_medal += p.ti_sim[i];
        tb_medal += p.tb_sim[i];
        ta_medal += p.ta_sim[i];
        gun_medal += p.gun_sim[i];
        td_medal += p.td_sim[i];
        missileboat_medal += p.missileboat_sim[i];
    }

    out << endl << "\tT/F: " << p.getmedal(tf_medal);
    out << endl << "\tT/I: " << p.getmedal(ti_medal);
    out << endl << "\tT/B: " << p.getmedal(tb_medal);
    out << endl << "\tT/A: " << p.getmedal(ta_medal);
    out << endl << "\tGUN: " << p.getmedal(gun_medal);
    out << endl << "\tT/D: " << p.getmedal(td_medal);
    out << endl << "\tMissile Boat: " << p.getmedal(missileboat_medal);

    out << endl << "Active Battle:\t" << (int)p.activebattle + 1;	// 616		(value: 00-xx? aka the last completed battle)

    for(int i=0; i<p.battlestatus.size(); i++) {
        out << endl << "Battle " << i+1 << " status:\t"; // 617-623	(value: active: 01, killed/captured: 02, complete: 03, killed/captured: 04)
        switch(p.battlestatus[i]) {
        case 0x1:
            out << "active. Last mission: " << (int) p.missionchoose[i];
            break;
        case 0x3:
            out << "completed. Last mission: " << (int) p.missionchoose[i];
            break;
        case 0x2:
        case 0x4:
            out << "captured or killed. Last mission: " << (int) p.missionchoose[i];
            break;
        default:
            out << "unknown";
        }
    }

    out<< endl << p.lasersfired << " Lasers fired, " << p.laserhits << " Lasers hit";
    if(p.lasersfired) out << " (" << (100*p.laserhits) / p.lasersfired << "%)";
    out << endl << p.warheadsfired << " Warheads fired, " << p.warheadhits << " Warheads hit";
    if(p.warheadsfired) out << " (" << (100*p.warheadhits) / p.warheadsfired << "%)";
    out << endl << "Total kills:\t" << p.total
        << endl << "Ships Captured:\t" << p.captured
        << endl << "Ships Lost:\t" << p.lost;

    out << endl << "Killdetails:";
    for(int i=0; i<p.kills.size(); i++)
        out << endl <<  p.shipnames[i] << ":\t" << p.kills[i];

    int training = 1;
    for(int i=0; i<p.trainingpoints.size(); ++i) {
        if(p.trainingpoints[i]) {
            out << endl << "Training " << training << ":\t" << p.trainingpoints[i] << " points";
            training++;
        }
    }

    int battle = 1;
    for(int i=0; i<p.battlepoints.size(); ++i) {
        if(p.battlepoints[i]) {
            out << endl << "Battlemission " << battle << ":\t" << p.battlepoints[i] << " points";
            battle++;
        }
    }
    return out;
}

int main(int argc, char* argv[])
{
    if(argc != 2) {
        cerr << "Please name a pilot file as parameter" << endl;
        return -1;
    }
    string filename(argv[1]);
    Pilot p(filename);
    cout << p << endl;
}