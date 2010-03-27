// This code is GPL, blah blah
//
// Writen for TheManaWorld by Chuck Miller, A.K.A. Kage

#include <cstring>
#include <iostream>
#include <stdlib.h>
#include <string>

using namespace std;

bool useStorage;
int itemID;

int itemCount(string itemData)
{
    int counter = 0;
    int pointer = 0;

    do
    {
        int ending = itemData.find(',', pointer + 1);
        if (ending == string::npos)
            ending = itemData.size();
        if (counter == 1 && atoi(itemData.substr(pointer, ending - pointer).c_str()) != itemID)
            return 0;
        if (counter == 2)
            return atoi(itemData.substr(pointer, ending - pointer).c_str());
        counter++;
    } while ((pointer = itemData.find(',',pointer) + 1) != string::npos + 1);

    return 0;
}

int parseItemData(string &items)
{
    int counter = 0;
    int pointer = 0;

    int total = 0;

    do
    {
        int ending = items.find(' ', pointer + 1);
        if (ending == string::npos)
            ending = items.size();
        total += itemCount(items.substr(pointer, ending - pointer));
        counter++;
    } while ((pointer = items.find(' ',pointer) + 1) != string::npos + 1);

    return total;
}

void parseLine(string &line)
{
    int counter = 0;
    int pointer = 0;

    string AccountId, Name, Items;

    do
    {
         if (!useStorage)
         {
             if (counter == 1 || counter == 2 || counter == 15)
             {         
                 int ending = line.find('\t', pointer + 1);
                 if (ending == string::npos)
                      ending = line.size();
                 switch (counter)
                 {
                    case 1:
                         AccountId = line.substr(pointer,ending - pointer);
                         break;
                    case 2:
                         Name = line.substr(pointer, ending - pointer);
                         break;
                    case 15:
                         Items = line.substr(pointer, ending - pointer);
                 }
            }
         }
         else if (useStorage)
         {
            int ending = line.find('\t', pointer + 1);
            if (ending == string::npos)
                ending = line.size();

            if (counter == 0)
                AccountId = line.substr(pointer,ending - pointer);
            else if (counter == 1)
                Items = line.substr(pointer,ending - pointer);            
         }
         
         counter++;
    } while ((pointer = line.find('\t',pointer) + 1) != string::npos + 1 && counter < 16);

    if ((counter = parseItemData(Items)) > 0)
        cout << "Account = " << AccountId << "; Name = \"" << Name << "\"; Count = " << counter << "\n";
}

void parseInput()
{
     string input;
     while (getline(cin, input))
     {
         parseLine(input);
     }
}

int main(int argc,char *argv[])
{
	if(argc < 2) 
        {
		printf("Usage: %s <item ID>\n", argv[0]);
                printf("Usage2: %s -s <item ID>\n", argv[0]);
                printf("e.g., %s 701\n", argv[0]);
                printf("Will return all users who own that item\n");
                printf("Option \"-s\" will expect storage files\n");
		exit(0);
        }
        if (strcmp(argv[1],"-s") == 0)
        {
            useStorage = true;            
            itemID = atoi(argv[2]);
        }
        else
        {
            useStorage = false;
            itemID = atoi(argv[1]);
        }
        parseInput();

	return 0;
}

