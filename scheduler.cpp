/*****************************************************************************
'scheduler.cpp' - The main program entry file for this project; contains main()
*****************************************************************************/

#include "scheduler.h"

using namespace std;

#define INPUTFILE "tasklist.txt"
#define OUTPUTFILE "output.csv"
#define STOVE_DIRTY 0
#define STOVE_CLEAN 2

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
        _stoveStatus = STOVE_CLEAN - 1;
    }
    Scheduler(const vector<Dish> &d)
    {
        for (int i = 0; i < d.size(); i++)
        {
            _dishes.push_back(d.at(i)); // Copy d into _dishes
        }
        _onStove = -1; // Nothing is on the stove
        _time = 0; // Begin at 0 "seconds"/_time units
        _stoveStatus = STOVE_CLEAN - 1;
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
            out.close();
        }
        else
        {
            fatal_err("Output file could not be opened.", 6);
        }
    }
private:
    /* Schedule() - selects the next dish to be cooked
     *            - no arguments
     *            - returns index (in _dishes) of dish to be cooked
     */
    int Schedule()
    {
        int k = 0;
        while (_dishes.at(k).GetState() != READY && _dishes.at(k).GetState() != ONSTOVE)
        {
            k++;
            if (k == _dishes.size())
            {
                k = -1;
                break;
            }
        }
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
        /**** Check if a dish is arriving ****/
        for (int i = 0; i < _dishes.size(); i++)
        {
            // A Dish has "arrived" if its arrival time is equal to current "time"
            if (_dishes.at(i).GetArrival() == _time)
            {
                // Add to Remarks
                remarks += _dishes.at(i).GetName() + " arrives. ";

                Dish &d = _dishes.at(i);
                Task * t = d.GetNextTask();
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
        if (_onStove > -1 && _onStove < _dishes.size() && _dishes.at(_onStove).GetState() == READY)
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

            // Do Tasks
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
                //_dishes.erase(_dishes.begin() + i);
                _dishes.at(i).SetState(DONE);
                n -= 1; // decrease number of dishes not done yet
            }
            else if (t->GetType() == COOK)
            {
                _dishes.at(i).SetState(READY);
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
    int _time;
    int _stoveStatus; // Stove status (from DIRTY to CLEAN, with 1 step in between)
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
                if (at == 0)
                {
                    input.close();
                    fatal_err("Input file is corrupted. Invalid arrival _time.", 3);
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
    exit(code);
}
