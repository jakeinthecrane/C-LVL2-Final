#include <iostream>  // our list of preprocessor directives loaded with functions from various libraries that have selectevely chosen for this specific program to consider data integrity.  
#include <stdexcept>  
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <memory>
#include <thread>
#include <atomic>
#include <chrono>
#include <limits> // directive to provide various functions in this case we use it to clear input buffer after taking value from cin. Better input handling.. 

using namespace std;

// Base class demonstrating abstraction to only show essential features.  Here is a blueprint form managing expenses without implementing the actual logic.  
class ExpenseBase
{
public: // Abstract interface utilizing polimorphism to define actions and force implementations for better data integrity. 
    virtual void addExpense(const string& category, const string& inputAmount) = 0; // Pure virtual method declared with = 0 to indicate that it doesn't have implementation.
    virtual void displayExpenses() const = 0; // Pure virtual method is only used as a placeholder forcing derived classes to provide an implementation.  
    virtual void calculateTotal() const = 0; // Pure virtual method.  Also you cannot instantiate the base class directly.  
    virtual ~ExpenseBase() = default; // Virtual destructor
};

// Derived class for managing expenses which inherits the ExpenseBase class to implement its abstract interface
class ExpenseManager : public ExpenseBase 
{
protected: // Encapsulation set to protected
    vector<pair<string, double>> expenses; // vector holding multiple expenses in this case a pair of expenses being a string and a double (category and amount). 

public:
    void addExpense(const string& category, const string& inputAmount) override // virtual function in the base class overriden to define this add expense function..
    {
        double amount;
        stringstream ss(inputAmount);
        if (!(ss >> amount)) 
        {
            throw invalid_argument("Error: Invalid input. Please enter a numeric value for the amount.");
        }
        if (amount < 0)
        {
            throw out_of_range("Error: Expense amount cannot be negative.");
        }
        expenses.emplace_back(category, amount);
        cout << "Added expense: " << category << " - $" << amount << endl;
    }

    void displayExpenses() const override // override the virtual function to implement this display expenses defined function 
    {
        if (expenses.empty()) 
        {
            cout << "No expenses recorded yet." << endl;
            return;
        }
        cout << "\nRecorded Expenses:" << endl;
        for (const auto& expense : expenses)
        {
            cout << "- " << expense.first << ": $" << expense.second << endl;
        }
    }

    void calculateTotal() const override 
    {
        if (expenses.empty()) 
        {
            throw runtime_error("Error: No expenses recorded. Please add expenses before calculating the total.");
        }

        const int numThreads = 4; // threads we are using.  2 threads per processor core.  my cpu has 4 cores so only half available logic threads are being used.  
        vector<double> partialSums(numThreads, 0.0);
        vector<thread> threads;

        auto sumExpenses = [&](int threadIndex, size_t start, size_t end) // lambda function used to calculate the toal of expenses for a specific rane of entries in 
                                                                          // the expense vector.. enables asynchronous programming
            {
            double sum = 0.0;
            for (size_t i = start; i < end; ++i)
            {
                sum = sum + expenses[i].second;
            }
            partialSums[threadIndex] = sum;
            };

        size_t expensesPerThread = expenses.size() / numThreads; // thread distribution formula set in a for loop condition to iterate until all threads have been assigned
        for (int i = 0; i < numThreads; ++i)
        {
            size_t start = i * expenses.size() / numThreads;
            size_t end = (i + 1) * expenses.size() / numThreads;
            threads.emplace_back(sumExpenses, i, start, end);
        }

        for (auto& t : threads) 
        {   // for loop makes main thread wait for all working threads to join here before proceeding.
            t.join();
        }

        double total = 0.0;
        for (const auto& sum : partialSums) // all partialSums threads get summed up here. 
        {
            total = total + sum; 
        }

        cout << "\nTotal spending: $" << total << endl;
    }
};

// Derived class for file management
class FileManager : public ExpenseManager 
{
public:
    void saveExpensesToFile(const string& filename) const 
    {
        ofstream file(filename);
        if (file.is_open()) {
            for (const auto& expense : expenses) {
                file << expense.first << " " << expense.second << "\n";
            }
            cout << "Expenses saved to file: " << filename << endl;
        }
        else
        {   // throwing an exception here vs the ifstream since it is more critical for this type of operation where we should address it.
            throw runtime_error("Error: Unable to save expenses to file.");
        }
    }

    void loadExpensesFromFile(const string& filename) 
    {
        ifstream file(filename);
        if (file.is_open())
        {
            string category;
            double amount;
            while (file >> category >> amount) 
            {
                expenses.emplace_back(category, amount);
            }
            cout << "Loaded expenses from file: " << filename << endl;
        }
        else
        {   // In this case we can create a more graceful UX and simply let the user there isn't any data rather than jarring them with an error message...
            cout << "No existing file found. Starting fresh." << endl;
        }
    }
};

// Utility class for managing UI. This class focuses on keeping the code more user focused keeping the business logic separate.  
class ExpenseTrackerUI : public FileManager 
{
private:
    atomic<bool> saveCompleted{ false };

public:
    void displayInstructions() const 
    {
        cout << "Welcome to the Personal Expense Tracker!" << endl;
        cout << "Organize your finances with ease.\n";
        cout << "1. Add Expense\n2. Display Expenses\n3. Calculate Total\n4. Save & Exit\n\n";
    }

    void menu() 
    {
        string filename = "expenses.txt";
        loadExpensesFromFile(filename);

        while (true) 
        {
            displayInstructions();
            cout << "Choose an option: ";
            int choice;
            cin >> choice;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');

            try 
            {
                if (choice == 1) 
                {
                    string category, inputAmount;
                    cout << "Enter category: ";
                    getline(cin, category);
                    cout << "Enter amount: $";
                    cin >> inputAmount;
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    addExpense(category, inputAmount);
                }
                else if (choice == 2)
                {
                    displayExpenses();
                }
                else if (choice == 3) 
                {
                    calculateTotal();
                }
                else if (choice == 4) 
                {
                    saveToFileAsync(filename); // begins a detatched thread to perform the file-saving operation asynchronously in the background. 
                    while (!saveCompleted.load()) // while loop checks periodically to see if the saveCompleted confirms true throught the atomic function via load()..
                    {       // uses the timer to stand-by and wait to see if the saveCompleted returns true.  Cycling not using CPU resources.
                        this_thread::sleep_for(chrono::milliseconds(50));
                    }
                    break;
                }
                else
                {
                    cout << "Invalid choice. Try again.\n";
                }
            }
            catch (const exception& e) 
            {
                cout << e.what() << endl;
            }
        }
    }

    void saveToFileAsync(const string& filename)
    {
        saveCompleted = false;
        thread saveThread([this, filename]() 
            {
            try
            {
                saveExpensesToFile(filename);
                saveCompleted = true;
            }
            catch (const exception& e) 
            {
                cout << e.what() << endl;
            }
            });
        saveThread.detach();
    }
};

// Main function
int main()
{
    unique_ptr<ExpenseTrackerUI> tracker = make_unique<ExpenseTrackerUI>(); // smart pointer used to call ExpenseTrackerUI.  We're going to use the pointer here to 
    tracker->menu(); // tracker object calls menu function to take over the process of the order of operations...
    return 0;
}
