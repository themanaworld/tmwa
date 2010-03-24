#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fstream>
#include <iostream>

#include "mmo.h"
#include "athena_text.h"

#define ATHENA_FILE "save/athena.txt"
#define ACCREG_FILE "save/accreg.txt"

long long countAthena()
{
    long long zeny = 0;
    int total = 0;
    std::string input;
    std::ifstream fp(ATHENA_FILE);
    char *buffer = new char[65536];

    while (fp.good())
    {
        std::getline(fp,input);
        mmo_charstatus *thisChar = new struct mmo_charstatus;

        strcpy(buffer,input.c_str());

        if (mmo_char_fromstr(buffer, thisChar))
        {
            total++;
            zeny += thisChar->zeny;
        }
        else
            std::cout << "Could not parse line \"" << buffer << "\"\n";

        delete thisChar;
    }


    std::cout << "Parsed a total of " << total << " lines in " << ATHENA_FILE << std::endl;

    delete [] buffer;
    fp.close();

    return zeny;
}

long long countAccReg()
{
     long long zeny = 0;
     int total = 0;
     std::ifstream fp(ACCREG_FILE);
     char *buffer = new char[65536];
     while (fp.good())
     {
         std::string line;
         std::getline(fp, line);
         struct accreg *reg = new struct accreg;

         strcpy(buffer, line.c_str());

         if (accreg_fromstr(buffer, reg))
         {
            total++;
            for (int i = 0; i < reg->reg_num; i++)
            {
                if (strcmp(reg->reg[i].str,"#BankAccount") == 0)
                {
                    zeny += reg->reg[i].value;
                }
            }
         }
         else
         {
             std::cout << "Could not parse line: \"" << buffer << "\"\n";
         }

         delete reg;
     }

    std::cout << "Parsed a total of " << total << " lines in " << ACCREG_FILE << std::endl;

     delete [] buffer;
     fp.close();

     return zeny;
}

int main()
{
    long long count = 0;
    count = countAthena();
    count += countAccReg();

    std::cout << "There is a total of " << count << " zeny on this server!" << std::endl;

    return 0;
}

