/*****************************************************************************
'scheduler.cpp' - The main program entry file for this project; contains main()
*****************************************************************************/

#include "scheduler.h"

using namespace std;

#define INPUTFILE "tasklist.txt"
#define OUTPUTFILE "output.csv"
#define PERFLOGFILE "perf.log"
#define STOVE_DIRTY 0
#define STOVE_CLEAN 2
#define QUEUE_COUNT 10
#define BOOST_QUANTUM 120 // Time interval for priority boost

//#define DEBUG

void fatal_err(const string &s, int code);

class Scheduler
{
public:
    /*
     * Constructors
     */
    Scheduler()
    {
        _onStove = -1; // Nothing is on the stove
        _time = 0; // Begin at 0 "seconds"/_time units
        _stoveUtil = 0;
        _stoveStatus = STOVE_CLEAN - 1;
        for (int i = 0; i < QUEUE_COUNT; i++)
        {
            vector<int> q;
            _mfqs[i] = q;
        }
        _chosenOne = 9; // Choose highest priority queue by default
        _quantum = 1; // By default, interrupt every 1 "second"
    }
    Scheduler(const vector<Dish> &d)
    {
        for (int i = 0; i < d.size(); i++)
        {
            _dishes.push_back(d.at(i)); // Copy d into _dishes
        }
        for (int i = 0; i < QUEUE_COUNT; i++)
        {
            vector<int> q;
            _mfqs[i] = q;
        }
        cout << _mfqs[10].size() << endl;
        _onStove = -1; // Nothing is on the stove
        _time = 0; // Begin at 0 "seconds"/_time units
        _stoveUtil = 0;
        _stoveStatus = STOVE_CLEAN - 1;
        _chosenOne = 9; // Choose highest priority queue by default
        _quantum = 1; // By default, interrupt every 1 "second"
    }

    vector<Dish>& GetDishes() // getter for Dishes not yet arrived (reference)
    {
        return _dishes;
    }
    Dish * GetOnStove() // getter for dish on stove (reference)
    {
        return (_onStove > -1) ? &(_dishes.at(_onStove)) : NULL;
    }
    int GetTime() // getter for time
    {
        return _time;
    }
    void Sim() // Begin simulation
    {
        int n = _dishes.size();
        ofstream out(OUTPUTFILE, ofstream::out);
        if (out.is_open())
        {
            // Print CSV headers
            out << "Time, Stove, Ready, Assistants, Remarks" << endl;
            while (n > 0)
            {
                n = Proceed(n, out);
            }
            // Write last line of output file
            out << ++_time << ", -- Idle --, , , Cleaning stove." << endl;
            // Close file stream
            out.close();
        }
        else
        {
            fatal_err("Output file could not be opened.", 6);
        }
        // Print out performance metric to log file
        out.open(PERFLOGFILE, ofstream::out);
        if (out.is_open())
        {
            // Write log header
            out << "Scheduling Performance Log" << endl;
            // Write metrics
            out << "Total Simulated Time          : " << _time << endl;
            out << "Stove Utilization Time        : " << _stoveUtil << endl;
            out << "Stove Idle Time               : " << _time - _stoveUtil << endl;
            // Get weighted average waiting time
            // WEIGHT = PRIORITY * WAITING TIME
            float w = 0.0;
            int totalP = 0;
            for (int i = 0; i < _dishes.size(); i++)
            {
                totalP += _dishes.at(i).GetPriority();
                w += _dishes.at(i).GetPriority() * _dishes.at(i).GetWaitingTime();
            }
            w /= totalP;
            out << "Weighted Average Waiting Time : " << w << endl;
            // Close file stream
            out.close();
        }
        else
        {
            fatal_err("Performance log file could not be opened.\nThe simulation still went through, but metrics were not recorded.", 6);
        }
    }
private:
    /* Schedule() - selects the next dish to be cooked
     *            - no arguments
     *            - returns index (in _dishes) of dish to be cooked
     */
    int Schedule()
    {
        /*** Preemption Operations ***/
        if (_onStove > -1)
        {
            // Preemption must occur
            // Demote currently cooking dish to lower queue

            // Temporarily Remove from scheduling queue to move somewhere else
            int y;
            bool notDone = true;
            for (y = 0; y < QUEUE_COUNT && notDone; y++)
            {
                for (int z = 0; z < _mfqs[y].size(); z++)
                {
                    if (_mfqs[y][z] == _onStove)
                    {
                        _mfqs[y].erase(_mfqs[y].begin() + z);
                        notDone = false;
                        y -= 1;
                        break;
                    }
                }
            }
            int lower = (y == 0) ? 0 : y - 1;
            _mfqs[lower].push_back(_onStove);
            _dishes.at(_onStove).SetPriority(lower + 1);

            #ifdef DEBUG
            cout << "$ Demote Dish " << _onStove << " from level " << y << " to " << lower << endl;
            #endif
        }

        int k = -1;
        _chosenOne = 9; // Bias for higher priority
        int cycleStart = _chosenOne;
        // Look for the first nonempty queue, cycling downwards
        do
        {
            // If queue is empty, cycle downward to next queue
            if ( _mfqs[_chosenOne].size() == 0 )
            {
                _chosenOne = (_chosenOne == 0) ? QUEUE_COUNT - 1 : _chosenOne - 1;
                continue;
            }

            /*** Select a task from queue ***/

            // Round Robin for priority level 0
            //  This is ensured because once a Dish/Process has been demoted
            //  to level 0, it can no longer be demoted further
            // FCFS for priority levels 1 and up
            int x = 0;
            k = _mfqs[_chosenOne][x];
            while( _dishes.at(k).GetState() != READY && _dishes.at(k).GetState() != ONSTOVE)
            {

                #ifdef DEBUG
                cout << "  X: " << x << "  STATE:  " << _dishes.at(k).GetState() << "  ";
                #endif

                x++;
                if (x > _mfqs[_chosenOne].size() - 1)
                {
                    k = -1;
                    break;
                }
                else
                {
                    k = _mfqs[_chosenOne][x];
                }
            }

            #ifdef DEBUG
            cout << "  K: " << k << "  ";
            #endif

            if (k > -1)
            {
                break;
            }
            _chosenOne = (_chosenOne == 0) ? QUEUE_COUNT - 1 : _chosenOne - 1;
        }
        while(_chosenOne != cycleStart);

        // Set time quantum
        // -- Lower priority level queues => higher quantum
        //     because they are less likely to be selected.
        switch(_chosenOne)
        {
            case 9:
                _quantum = 2;
            break;
            case 8:
                _quantum = 3;
            break;
            case 7:
                _quantum = 4;
            break;
            case 6:
                _quantum = 6;
            break;
            case 5:
                _quantum = 7;
            break;
            case 4:
                _quantum = 8;
            break;
            case 3:
                _quantum = 10;
            break;
            case 2:
                _quantum = 11;
            break;
            case 1:
                _quantum = 12;
            break;
            default:
                _quantum = 14;
        }

        #ifdef DEBUG
        cout << "  CHOOSE:  " << k;
        cout << "  C: " << _chosenOne << "  CS: " << cycleStart;
        cout << "  Q:  " << _quantum << endl;
        #endif

        // Next cycle should start with queue below this one
        //_chosenOne = (_chosenOne == 0) ? QUEUE_COUNT - 1 : _chosenOne - 1;

        return k;
    }
    /* Proceed() - moves the simulation one step forward in time
     *           - arguments are number of Dishes not done & output stream
     *           - returns number of Dishes not done;
     */
    int Proceed(int n, ostream &out) // argument : output stream where output is to be printed
    {
        string remarks; // Stores remarks string
        _time++; // Time travel (1 second ahead)
        _quantum--; // Go closer to next quantum

        #ifdef DEBUG
        cout << _time << "  ";
        #endif

        #ifdef DEBUG
        for (int i = 0; i < QUEUE_COUNT; i++)
        {
            cout << "Q" << i << ": ";
            for (int j = 0; j < _mfqs[i].size(); j++)
            {
                cout << _mfqs[i][j] << ", ";
            }
        }
        cout << endl;
        #endif

        /**** Check if a dish is arriving ****/
        for (int i = 0; i < _dishes.size(); i++)
        {
            // A Dish has "arrived" if its arrival time is equal to current "time"
            if (_dishes.at(i).GetArrival() == _time)
            {
                // Add to Remarks
                remarks += _dishes.at(i).GetName() + " arrives. ";

                Dish &d = _dishes.at(i); // get reference to target Dish

                // Add Process Index to scheduling queue, initially equal to priority
                _mfqs[d.GetPriority() - 1].push_back(i);

                Task * t = d.GetNextTask();
                // Change State of Dish so it is not passed over by
                //  later instructions
                if (t->GetType() == COOK)
                {
                    d.SetState(READY); // Ready for cooking
                }
                else
                {
                    d.SetState(PREPPING); // Must be prepped
                }
            }
        }
        /**** Select which one to cook next ****/

        // TESTING >>=>> Use First Come First Serve
        int k;
        if (_quantum > 0 && _onStove > -1 && _onStove < _dishes.size() && _dishes.at(_onStove).GetState() == READY)
        {
            k = _onStove;
        }
        else
        {
            k = Schedule();
        }

        // if Stove is clean or dish isn't changed, proceed normally
        if (k == _onStove || _stoveStatus == STOVE_CLEAN)
        {
            _onStove = k;
            // The stove is "dirty" now iff. it is not empty
            if (_onStove > -1) _stoveStatus = STOVE_DIRTY;
        }
        // Stove is dirty and dish needs to be changed
        else if (_stoveStatus == STOVE_DIRTY)
        {
            _onStove = -1;
            remarks += "Cleaning stove. ";
            _stoveStatus += 1; // 1 step towards a clean stove
        }
        // Dish needs to be changed and stove is almost clean
        else
        {
            _onStove = -1;
            remarks += "Preheating stove. ";
            _stoveStatus += 1; // Stove is clean next time
        }

        if (_onStove > -1)
        {
            _dishes.at(_onStove).SetState(ONSTOVE);
        }

        /**** Cook ****/

        // Print Time
        out << _time << ", ";

        // Print Dish on stove
        if (_onStove > -1)
        {
            out << _dishes.at(_onStove);
            _stoveUtil += 1;
        }
        else
        {
            out << "-- Idle --";
        }
        out << ", ";
        // Print Ready
        for (int i = 0; i < _dishes.size(); i++)
        {
            if (_dishes.at(i).GetState() == READY && _dishes.at(i).GetState() != ONSTOVE)
            {
                out << _dishes.at(i) << "   ";
            }
        }
        out << ", ";
        // Print Assistants/Prep
        for (int i = 0; i < _dishes.size(); i++)
        {
            if (_dishes.at(i).GetState() == PREPPING)
            {
                out << _dishes.at(i) << "   ";
            }
        }
        out << ", ";

        for (int i = 0; i < _dishes.size(); i++)
        {
            if (_dishes.at(i).GetState() == NOTARRIVED || _dishes.at(i).GetState() == DONE)
            {
                continue;
            }

            Task * t = _dishes.at(i).GetNextTask();
            Task * prevT = t;

            /*** Do Tasks ***/
            dish_state dS = _dishes.at(i).GetState();
            if (dS == ONSTOVE || dS == PREPPING)
            {
                t->Step();
            }
            t = _dishes.at(i).GetNextTask();

            // Set States based on next task
            if (t == NULL)
            {
                // No more tasks in recipe -- dish is done
                remarks += _dishes.at(i).GetName() + " is Done. ";
                // Set state to DONE, so it is ignored
                _dishes.at(i).SetState(DONE);
                n -= 1; // decrease number of dishes not done yet
                // Remove from scheduling queue
                bool notDone = true;
                for (int y = 0; y < QUEUE_COUNT && notDone; y++)
                {
                    for (int z = 0; z < _mfqs[y].size(); z++)
                    {
                        if (_mfqs[y][z] == i)
                        {
                            _mfqs[y].erase(_mfqs[y].begin() + z);
                            notDone = false;
                            break;
                        }
                    }
                }
            }
            else if (t->GetType() == COOK)
            {
                _dishes.at(i).SetState(READY);
                // In Ready queue, i.e. Dish is WAITING; increment waiting time
                _dishes.at(i).Wait();
                // Check if just finished from PREP stage
                if (prevT->GetType() == PREP)
                {
                    // Equivalent of IO Blocking
                    //   'Promote' Dish/Process one level higher
                    // Temporarily remove from scheduling queue to move elsewhere
                    bool notDone = true;
                    int y;
                    for (y = 0; y < QUEUE_COUNT && notDone; y++)
                    {
                        for (int z = 0; z < _mfqs[y].size(); z++)
                        {
                            if (_mfqs[y][z] == i)
                            {
                                _mfqs[y].erase(_mfqs[y].begin() + z);
                                notDone = false;
                                y -= 1;
                                break;
                            }
                        }
                    }
                    // Push into one level higher
                    int higher = (y == QUEUE_COUNT - 1) ? QUEUE_COUNT - 1 : y + 1;
                    _mfqs[higher].push_back(i);
                    _dishes.at(i).SetPriority(higher + 1);

                    #ifdef DEBUG
                    cout << endl << "$ Promote Dish " << i << " from level " << y << " to " << higher << endl;
                    #endif
                }
            }
            else if (t->GetType() == PREP)
            {
                if (dS == ONSTOVE)
                {
                    _dishes.at(i).SetState(MOVING);
                }
                else
                {
                    _dishes.at(i).SetState(PREPPING);
                }
            }
        }
        // Print Remarks
        out << remarks << endl;

        /*** Return number of dishes not yet done ***/
        return n;
    }
    vector<Dish> _dishes;
    int _onStove; // Index in _dishes of Dish currently On Stove
    int _time; // Simulated time
    int _stoveUtil; // stove utilization time
    int _stoveStatus; // Stove status (from DIRTY to CLEAN, with 1 step in between)

    int _quantum; // Current time quantum
    // Multiple Feedback queues
    vector<int> _mfqs[QUEUE_COUNT]; // an array of size QUEUE_COUNT of int vectors (ref)
                                    // each int vector is a queue.
    int _chosenOne; // Index of last chosen queue
};

int main()
{
    cout << endl << "CS 140 Machine Problem" << endl;
    cout << "----------------------------" << endl;
    cout << "Vincent Fiestada | 201369155" << endl << endl;

    Scheduler core = Scheduler();

    // Opening input file
    ifstream input(INPUTFILE);
    string taskDesc;
    if (input.is_open())
    {
        while (getline(input, taskDesc))
        {
            // Parse by finding the space
            size_t x = taskDesc.find(" ");
            if (x != string::npos)
            {
                // The name is the substr before the space
                // The number is the substr after the space
                int at = atoi(taskDesc.substr(x + 1).c_str()); // convert string to int
                if (at < 1)
                {
                    input.close();
                    fatal_err("Input file is corrupted. Invalid arrival time.", 3);
                }
                Dish d = Dish(taskDesc.substr(0, x), at, 0);
                // Open recipe file and add recipe steps to dish d
                string recipeFilename = "recipes/" + d.GetName() + ".txt";
                ifstream recipeFile(recipeFilename.c_str());
                if (recipeFile.is_open())
                {
                    string recipeLine;
                    size_t y;
                    if (getline(recipeFile, recipeLine))
                    {
                        // Parse by finding the space
                        y = recipeLine.find(" ");
                        if (y != string::npos)
                        {
                            // The priority is the number after the space
                            int p = atoi(recipeLine.substr(y + 1).c_str()); // convert string to int
                            // Check if priority is valid
                            if (p < 1 || p > QUEUE_COUNT)
                            {
                                fatal_err("Recipe file '" + recipeFilename + "' is corrupted. Dish priority is missing.", 5);
                            }
                            d.SetPriority(p);
                        }
                        else
                        {
                            // space not found, trigger error
                            fatal_err("Recipe file '" + recipeFilename + "' is corrupted. Dish priority is missing", 5);
                        }
                    }
                    while(getline(recipeFile, recipeLine))
                    {
                        // Parse by finding the space
                        y = recipeLine.find(" ");
                        if (y != string::npos)
                        {
                            // Step description is before the space
                            int ty = (recipeLine.substr(0, y) == "cook") ? COOK : PREP;
                            // Step time is the number after the space
                            int ti = atoi(recipeLine.substr(y + 1).c_str()); // convert string to int
                            d.GetRecipe().push_back(Task(ty, ti)); // Push into recipe vector
                        }
                        else
                        {
                            // space not found, trigger error
                            fatal_err("Recipe file '" + recipeFilename + "' is corrupted.", 5);
                        }
                    }
                    recipeFile.close();

                    // push into dishes
                    core.GetDishes().push_back(d);
                }
                else
                {
                    fatal_err("Recipe file '" + recipeFilename + "' not found.", 4);
                }
            }
            else
            {
                input.close();
                fatal_err("Input file is corrupted. Space delimiter missing.", 2);
            }
        }
        input.close();
    }
    else
    {
        fatal_err("File could not be opened.", 1);
    }

    core.Sim();

    return 0;
}

void fatal_err(const string &s, int code)
{
    cout << "FATAL ERR: " << s << endl;
    cout << "The error code is " << code << " in case you need it." << endl;
    exit(code);
}
