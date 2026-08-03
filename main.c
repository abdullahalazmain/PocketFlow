#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define FILENAME "users.dat"

// ---------------- DATA STRUCTURE ----------------
struct User {
    char phone[50];
    char password[50];
    double balance; // <-- NEW: initial balance field
};

// ---------------- CLEAR SCREEN AND HEADER ----------------

void clearScreenAndShowBanner() {
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif

    printf("\n");
    printf(" +===================================================================+\n");
    printf(" |                   P O C K E T F L O W                             |\n");
    printf(" |          Next-Gen Digital Wallet & Finance                        |\n");
    printf(" +===================================================================+\n\n");
}

// ---------------- VALIDATE PHONE NUMBER (BD , 11 DIGITS, 013-019) ----------------

int validatePhoneNumber(char number[])
{
    int i;

    if (strlen(number) != 11) {
        printf("Invalid! Number must be exactly 11 digits.\n");
        return 0;
    }

    for (i = 0; i < 11; i++) {
        if (number[i] < '0' || number[i] > '9') {
            printf("Invalid! Number must contain digits only.\n");
            return 0;
        }
    }

    if(number[2] < '3' || number[2] > '9' || number[0] != '0' || number[1] !='1' )
    {
        printf("Invalid! Number must start with 013/014/015/016/018/019.\n");
        return 0;
    }

    return 1;
}

// ---------------- SIGN UP ----------------
void signUp()
{
    struct User newUser;

    newUser.balance = 0.00; // <-- every new account starts with 0.00


    // Beautiful header
    printf("\n--- SIGN UP ---\n");

    do
    {
        printf("Enter phone number : ");
        scanf("%49s", newUser.phone);
    } while (!validatePhoneNumber(newUser.phone));

    printf("Enter password: ");
    scanf("%49s", newUser.password);

    
    // check if phone already exists
    FILE *fp = fopen(FILENAME, "rb");
    if (fp != NULL)
    {
        struct User temp;
        while (fread(&temp, sizeof(struct User), 1, fp) == 1)
        {
            if (strcmp(temp.phone, newUser.phone) == 0)
            {
                printf("phone already exists! Try a different one.\n");
                fclose(fp);
                return;
            }
        }
        fclose(fp);
    }

    fp = fopen(FILENAME, "ab");
    if (fp == NULL)
    {
        printf("Error opening file!\n");
        return;
    }
    fwrite(&newUser, sizeof(struct User), 1, fp);
    fclose(fp);

    printf("Sign up successful! You can now sign in.\n");
}

// ---------------- SIGN IN ----------------
// loggedInUser pointer e matched user er FULL data (phone + balance shoho)
// fill kore dey, jate dashboard e egula use kora jay
int signIn(struct User *loggedInUser)
{
    char phone[50], password[50];
    struct User temp;
    FILE *fp;
    int found = 0;
    int attempts = 3;
    int chk = 0;

    printf("\n--- SIGN IN ---\n");
    printf("Enter phonenumber: ");
    scanf("%49s", phone);

    fp = fopen(FILENAME, "rb");
    if (fp == NULL)
    {
        printf("No users registered yet. Please sign up first.\n");
        return 0;
    }

    while (fread(&temp, sizeof(struct User), 1, fp) == 1)
    {
        if (strcmp(temp.phone, phone) == 0)
        {
            found = 1;
            break;
        }
    }
    fclose(fp);

    if (!found)
    {
        printf("phone not found. Please sign up first.\n");
        return 0;
    }

    while (attempts > 0)
    {
        printf("Enter password: ");
        scanf("%49s", password);

        if (strcmp(temp.password, password) == 0)
        {
            printf("Login successful! Welcome, %s.\n", phone);
            *loggedInUser = temp; // <-- matched user er data (balance shoho) copy kora hocche
            chk = 1;
            break;
        }
        else
        {
            attempts--;
            if (attempts > 0)
                printf("Wrong password! %d attempt(s) left.\n", attempts);
            else
                printf("Wrong password! No attempts left. Login failed.\n");
        }
    }

    return chk;
}

// ---------------- BANNER (project name design) ----------------
/*void printBanner()
{
    printf("=====================================\n");
    printf("            P O C K E T   F L O W    \n");
    printf("=====================================\n");
}*/

// ---------------- DASHBOARD HEADER (banner + balance) ----------------
// Eita dashboard e thakle call hobe -- screen clear kore
// banner + corner e balance show kore
void showDashboardHeader(double balance)
{
    clearScreenAndShowBanner();
    printf("Balance: %.2f\n", balance); // "corner" e balance -- console e ei line ta
    printf("-------------------------------------\n");
}

// ---------------- OPTION HEADER (banner only, NO balance) ----------------
// Kono option (Add Money, Cash Out etc.) er bhitore dhukle eita call hobe
// screen clear kore shudhu banner dekhabe, balance hide thakbe
void showOptionHeader()
{
    clearScreenAndShowBanner();
    printf("-------------------------------------\n");
}

// ---------------- UPDATE USER RECORD IN FILE ----------------
// Add Money, Cash Out, Payment -- shob money-feature e balance change
// hole eita call korte hobe, jate file e o updated data save hoy.
// Age discuss kora "fseek diye exact record e giye overwrite" pattern.
int updateUserInFile(struct User *user)
{
    FILE *fp = fopen(FILENAME, "r+b"); // read + write dutai lagbe
    if (fp == NULL)
    {
        printf("Error: could not open file to update balance.\n");
        return 0;
    }

    struct User temp;
    while (fread(&temp, sizeof(struct User), 1, fp) == 1)
    {
        if (strcmp(temp.phone, user->phone) == 0)
        {
            fseek(fp, -(long)sizeof(struct User), SEEK_CUR); // ek record pichone
            fwrite(user, sizeof(struct User), 1, fp);        // notun data overwrite
            fclose(fp);
            return 1;
        }
    }

    fclose(fp);
    printf("Error: user record not found in file.\n");
    return 0;
}

// ---------------- ADD MONEY -> Rakib ----------------

void addMoney(struct User *user)
{
    char agentNum[12];
    double amount;

    printf("\n~~~~~~~~~~>> ADD MONEY <<~~~~~~~~~~\n");

    // valid number na dile ashte thakbe, must valid number nite hobe
    do
    {
        printf("Enter Agent Number: ");
        scanf(" %11[^\n]", agentNum); // leading space -- leftover \n skip korbe
    } while (!validatePhoneNumber(agentNum));

    printf("Enter Amount to Add: ");
    scanf("%lf", &amount);

    if (amount <= 0)
    {
        printf("Your amount is too low. Try again.\n");
        return;
    }

    user->balance += amount; // memory te update

    if (updateUserInFile(user))
    {
        printf("\nSuccessfully added %.2f Taka via agent %s.\n", amount, agentNum);
        printf("New Balance: %.2f\n", user->balance);
    }
    else
    {
        user->balance -= amount; // file update fail korle rollback
        printf("Something went wrong. Balance not updated.\n");
    }
}
// feature/CASHOUT//
void cashOut(struct User *user)
{
    char agentNum[12];
    char password[50];
    double amount, charge, total;
    int attempts = 3;
    double CASH_OUT_RATE = 0.015;   // 1.5% cash out charge

    // ---------------- Screen 1: Number + Amount ----------------
    showOptionHeader();
    printf("><><><><><<< C A S H   O U T >>><><><><><\n");

    do
    {
        printf("Enter Number: ");
        scanf(" %11[^\n]", agentNum);   // shuru te space -- leftover \n skip korbe
    } while (!validatePhoneNumber(agentNum));

    printf("Enter Amount to cash out: ");
    scanf("%lf", &amount);

    if (amount <= 0)
    {
        printf("Your amount is too low. Try again.\n");
        return;
    }

    charge = amount * CASH_OUT_RATE;
    total = amount + charge;

    if (total > user->balance)
    {
        printf("Insufficient Balance! You need %.2f Taka but you have %.2f Taka.\n",
               total, user->balance);
        return;
    }

    // ---------------- Screen 2: Amount after charge ----------------
    showOptionHeader();
    printf("><><><><><<< C A S H   O U T >>><><><><><\n\n");
    printf("Amount after adding cash out charge: %.2f\n", total);
    printf("Enter to Continue...");
    getchar();   // leftover \n from scanf("%lf", ...) clear kore
    getchar();   // asol Enter key press er jonno wait kore

    // ---------------- Screen 3: PIN/Password check ----------------
    showOptionHeader(); 
    printf("><><><><><<< C A S H   O U T -Rakib >>><><><><><\n\n");
    while (attempts > 0)
    {
       
        printf("Enter your PIN: ");
        scanf("%49s", password);

        // Check if pin matches with the pin the owner enter to login
        if (strcmp(password, user->password) == 0)
        {
            user->balance -= total;

            if (updateUserInFile(user))
            {
                // ---------------- Screen 4: Success ----------------
                showOptionHeader();
                printf("Your Cash out is successfully cash out %.2f and your current balance (%.2f)!!!\n",
                       amount, user->balance);
            }
            else
            {
                user->balance += total;   // file update fail korle rollback
                printf("Something went wrong. Balance not updated.\n");
            }
            return;
        }
        else
        {
            attempts--;
            if (attempts > 0)
                printf("Wrong PIN! %d attempt(s) left. Try again.\n", attempts);
            else
                printf("Wrong PIN! No attempts left. Cash Out cancelled.\n");
        }
    }
}
// ---------------- DASHBOARD ----------------
// user pointer -- karon balance update hole (Add Money etc.) shei change
// dashboard e ferot ashle updated dekhate hobe
void dashboard(struct User *user)
{
    int choice;

    while (1)
    {
        showDashboardHeader(user->balance); // ekhane balance dekha jabe

        printf("1. Add Money\n");
        printf("2. Cash Out\n");
        printf("3. Send Money\n");
        printf("4. Transaction History\n");
        printf("5. Logout\n");
        printf("Choose an option: ");
        scanf("%d", &choice);

        switch (choice)
        {
        case 1:
            showOptionHeader(); // balance hide, banner thakbe
            addMoney(user);
            printf("\nPress Enter to return to dashboard...");
            getchar();
            getchar();
            break;

        case 2:
            showOptionHeader();
            cashOut(user);
            printf("\nPress Enter to return to dashboard...");
            getchar();
            getchar();
            break;

        case 3:
            showOptionHeader();
            printf("--- Send Money (friend's work here) ---\n");
            printf("\nPress Enter to return to dashboard...");
            getchar();
            getchar();
            break;

        case 4:
            showOptionHeader();
            printf("--- Transaction History (friend's work here) ---\n");
            printf("\nPress Enter to return to dashboard...");
            getchar();
            getchar();
            break;

        case 5:
            return; // dashboard theke ber hoye main menu e chole jabe

        default:
            printf("Invalid choice.\n");
        }
    }
}

// ---------------- MAIN MENU ----------------
int main()
{
    int choice;
    struct User currentUser; // <-- logged-in user er data ekhane thakbe

    while (1)
    {
        clearScreenAndShowBanner();
        printf("1. Sign Up\n");
        printf("2. Sign In\n");
        printf("3. Exit\n");
        printf("Choose an option: ");
        scanf("%d", &choice);

        switch (choice)
        {
        case 1:
            signUp();
            break;
        case 2:
            if (signIn(&currentUser) == 1)
            {
                dashboard(&currentUser);
            }
            break;
        case 3:
            printf("Exiting... Thank you!\n");
            return 0;
        default:
            printf("Invalid choice, try again.\n");
        }
    }

    return 0;
}
