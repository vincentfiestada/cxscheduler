/*****************************************************************************
'scheduler.cpp' - The main program entry file for this project; contains main()
*****************************************************************************/

#include "scheduler.h"

using namespace std;

#define INPUTFILE "tasklist.txt"

class Scheduler
{
public:
    Scheduler()
    {
        OnStove = -1;
        Time = 0;
    }
    Scheduler(const vector<Dish> &d)
    {
        for (int i = 0; i < d.size(); i++)
        {
            Dishes.push_back(d.at(i)); // Copy d into Dishes
        }
        OnStove = -1;
        Time = 0;
    }
    vector<Dish>& GetDishes()
    {
        return Dishes;
    }
    vector<Dish>& GetReady()
    {
        return Ready;
    }
    vector<Dish>& GetPrep()
    {
        return Prep;
    }
    int GetTime()
    {
        return Time;
    }
    void Sim() /* Begin simulation */
    {
        while (Dishes.size() > 0 || Ready.size() > 0 || Prep.size() > 0)
        {
            Proceed();
        }
    }
private:
    void Proceed()
    {
        Time++;
        cout << "Time: " << Time << endl;
        /**** Check if a dish is arriving ****/
        for (int i = 0; i < Dishes.size(); i++)
        {
            if (Dishes.at(i).GetArrival() == Time)
            {
                cout << Dishes.front().GetName() << " arrives" << endl;
                Ready.push_back(Dishes.front());
                Dishes.erase(Dishes.begin() + i);
            }
        }
        /**** Sort Ready Queue ****/
        /**** Cook ****/

        cout << "Ready: ";
        for (int j = 0; j < Ready.size(); j++)
        {
            cout << Ready.at(j).GetName() << ' ';
        }
        cout << endl;

        OnStove = 0;
        if (Ready.size() >= 1)
        {
            cout << "OnStove: " << Ready.at(OnStove).GetName() << endl;
        }

        Ready.clear();
    }
    vector<Dish> Dishes;
    vector<Dish> Ready;
    vector<Dish> Prep;
    int OnStove; // Index in Ready of Dish currently On Stove
    int Time;
};

void fatal_err(const string &s, int code);

int main()
{
    cout << endl << "CS 140 Machine Problem" << endl;
    cout << "----------------------------" << endl;
    cout << "Vincent Fiestada | 201369155" << endl << endl;

    cout << "Dishes to Prepare:" << endl;

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
                int at = atoi(taskDesc.substr(x + 1).c_str());
                if (at == 0)
                {
                    input.close();
                    fatal_err("Input file is corrupted. Invalid arrival time.", 3);
                }
                core.GetDishes().push_back(Dish(taskDesc.substr(0, x), at));
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
