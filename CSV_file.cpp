#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>

using namespace std;

class data_Generator
{
    public:
        fstream fout;

        void dataCreation()
        {

            // opens an existing csv file or creates a new file.

            fout.open("reportcard.csv", ios::out | ios::app );

            if (!fout.is_open()) 
            {
                cout << "Error: Unable to create file" << endl;
                exit;
            }

            srand(time(nullptr));

            for (int i = 0; i < 10000; i++) 
            {
                time_t timestamp = time(nullptr) + i;
                double speed1 = rand() % 601 + 1800;
                double speed2 = rand() % 601 + 1800;
                double speed3 = rand() % 601 + 1800;
            
                double pressure1 = (rand() % 50) / 1000.0 + 0.05;
                double pressure2 = (rand() % 50) / 1000.0 + 0.05;

                double voltage = rand() % 13 + 12 ;
                fout << timestamp << ", " << speed1 << ", " << speed2 << ", " << speed3 << ", " << pressure1 << ", " << pressure2 << ", " << voltage << endl;
                // sleep(1);
            }

            fout.close();

        }
};

int main() 
{   
    data_Generator data_gen;
    data_gen.dataCreation();
    return 0;
}
