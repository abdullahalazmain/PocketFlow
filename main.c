#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <time.h> // For OTP Random Generation

#define FILENAME "users.dat"
#define CLAIMS_FILE "claims.dat"

// ---------------- DATA STRUCTURE ----------------

// User structure
struct User {
    char phone[50];
    char password[50];
    double balance;
};

// OTP Claim Structure
struct Claim {
    char senderPhone[50];
    char receiverPhone[50];
    double amount;
    int otp;
    int status; // 0 = Pending, 1 = Claimed, 2 = Failed/Refunded, 3 = Archived
};

// ================= FUNCTION PROTOTYPES =================
void signUp();
int signIn(struct User *loggedInUser);
void addMoney(struct User *user);
void cashOut(struct User *user);
void mblRecharge(struct User *user);
void sendMoney(struct User *sender);
void showNotifications(struct User *currentUser);
int getPendingNotificationCount(char *userPhone);
void refundToSender(char *senderPhone, double amount);
int validatePhoneNumber(char number[]);
int updateUserInFile(struct User *user);
void showOptionHeader();
void showDashboardHeader(double balance);
void clearScreenAndShowBanner();
void dashboard(struct User *user);

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
    printf(" |            Next-Gen Digital Wallet & Finance                      |\n");
    printf(" +===================================================================+\n\n");
}

// ---------------- VALIDATE PHONE NUMBER (BD , 11 DIGITS, 013-019) ----------------

int validatePhoneNumber(char number[]) {
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

    if (number[2] < '3' || number[2] > '9' || number[0] != '0' || number[1] != '1') {
        printf("Invalid! Number must start with 013/014/015/016/018/019.\n");
        return 0;
    }

    return 1;
}

// ---------------- SIGN UP ----------------
void signUp() {
    struct User newUser;
    newUser.balance = 0.00; // Every new account starts with 0.00

    printf("\n--- SIGN UP ---\n");

    do {
        printf("Enter phone number : ");
        scanf("%49s", newUser.phone);
    } while (!validatePhoneNumber(newUser.phone));

    printf("Enter password: ");
    scanf("%49s", newUser.password);

    FILE *fp = fopen(FILENAME, "rb");
    if (fp != NULL) {
        struct User temp;
        while (fread(&temp, sizeof(struct User), 1, fp) == 1) {
            if (strcmp(temp.phone, newUser.phone) == 0) {
                printf("Phone already exists! Try a different one.\n");
                fclose(fp);
                return;
            }
        }
        fclose(fp);
    }

    fp = fopen(FILENAME, "ab");
    if (fp == NULL) {
        printf("Error opening file!\n");
        return;
    }
    fwrite(&newUser, sizeof(struct User), 1, fp);
    fclose(fp);

    printf("Sign up successful! You can now sign in.\n");

    printf("\n   Press Enter to return...");
    while (getchar() != '\n');
    getchar();
}

// ---------------- SIGN IN ----------------
int signIn(struct User *loggedInUser) {
    char phone[50], password[50];
    struct User temp;
    FILE *fp;
    int found = 0;
    int attempts = 3;
    int chk = 0;

    printf("\n--- SIGN IN ---\n");
    printf("Enter phone number: ");
    scanf("%49s", phone);

    fp = fopen(FILENAME, "rb");
    if (fp == NULL) {
        printf("No users registered yet. Please sign up first.\n");
        printf("\n   Press Enter to return...");
        while (getchar() != '\n'); getchar();
        return 0;
    }

    while (fread(&temp, sizeof(struct User), 1, fp) == 1) {
        if (strcmp(temp.phone, phone) == 0) {
            found = 1;
            break;
        }
    }
    fclose(fp);

    if (!found) {
        printf("Phone not found. Please sign up first.\n");
        printf("\n   Press Enter to return...");
        while (getchar() != '\n'); getchar();
        return 0;
    }

    while (attempts > 0) {
        printf("Enter password: ");
        scanf("%49s", password);

        if (strcmp(temp.password, password) == 0) {
            printf("Login successful! Welcome, %s.\n", phone);
            *loggedInUser = temp; // Copy full user data
            chk = 1;
            break;
        } else {
            attempts--;
            if (attempts > 0)
                printf("Wrong password! %d attempt(s) left.\n", attempts);
            else
                printf("Wrong password! No attempts left. Login failed.\n");
        }
    }
    sleep(1);

    return chk;
}

// ---------------- DASHBOARD HEADER ----------------
void showDashboardHeader(double balance) {
    clearScreenAndShowBanner();
    printf("Balance: %.2f BDT\n", balance);
    printf("-------------------------------------\n");
}

// ---------------- OPTION HEADER ----------------
void showOptionHeader() {
    clearScreenAndShowBanner();
    printf("-------------------------------------\n");
}

// ---------------- UPDATE USER RECORD IN FILE ----------------
int updateUserInFile(struct User *user) {
    FILE *fp = fopen(FILENAME, "r+b"); // Read and Write
    if (fp == NULL) {
        printf("Error: could not open file to update balance.\n");
        return 0;
    }

    struct User temp;
    while (fread(&temp, sizeof(struct User), 1, fp) == 1) {
        if (strcmp(temp.phone, user->phone) == 0) {
            fseek(fp, -(long)sizeof(struct User), SEEK_CUR); // Move backward 1 record
            fwrite(user, sizeof(struct User), 1, fp);        // Overwrite updated record
            fclose(fp);
            return 1;
        }
    }

    fclose(fp);
    printf("Error: user record not found in file.\n");
    return 0;
}

// ---------------- ADD MONEY ----------------
void addMoney(struct User *user) {
    char agentNum[12];
    double amount;

    printf("\n~~~~~~~~~~>> ADD MONEY <<~~~~~~~~~~\n");

    do {
        printf("Enter Agent Number: ");
        scanf(" %11[^\n]", agentNum);
    } while (!validatePhoneNumber(agentNum));

    printf("Enter Amount to Add: ");
    scanf("%lf", &amount);

    if (amount <= 0) {
        printf("Your amount is too low. Try again.\n");
        return;
    }

    user->balance += amount;

    if (updateUserInFile(user)) {
        printf("\nSuccessfully added %.2f Taka via agent %s.\n", amount, agentNum);
        printf("New Balance: %.2f BDT\n", user->balance);
    } else {
        user->balance -= amount; // Rollback
        printf("Something went wrong. Balance not updated.\n");
    }
}

// ---------------- CASH OUT ----------------
void cashOut(struct User *user) {
    char agentNum[12];
    char password[50];
    double amount, charge, total;
    int attempts = 3;
    double CASH_OUT_RATE = 0.015; // 1.5% charge

    showOptionHeader();
    printf("><><><><><<< C A S H   O U T >>><><><><><\n");

    do {
        printf("Enter Agent number : ");
        scanf("%49s", agentNum);
    } while (!validatePhoneNumber(agentNum));

    printf("Enter Amount to cash out: ");
    scanf("%lf", &amount);

    if (amount <= 0) {
        printf("Your amount is too low. Try again.\n");
        return;
    }

    charge = amount * CASH_OUT_RATE;
    total = amount + charge;

    if (total > user->balance) {
        printf("Insufficient Balance! You need %.2f Taka but you have %.2f Taka.\n",
               total, user->balance);
        return;
    }

    showOptionHeader();
    printf("><><><><><<< C A S H   O U T >>><><><><><\n\n");
    printf("Amount after adding cash out charge: %.2f BDT\n", total);
    printf("Enter to Continue...");
    getchar();
    getchar();

    showOptionHeader(); 
    printf("><><><><><<< C A S H   O U T >>><><><><><\n\n");
    while (attempts > 0) {
        printf("Enter your PIN: ");
        scanf("%49s", password);

        if (strcmp(password, user->password) == 0) {
            user->balance -= total;

            if (updateUserInFile(user)) {
                showOptionHeader();
                printf("Successfully cashed out %.2f BDT! Current balance: %.2f BDT\n",
                       amount, user->balance);
            } else {
                user->balance += total; // Rollback
                printf("Something went wrong. Balance not updated.\n");
            }
            return;
        } else {
            attempts--;
            if (attempts > 0)
                printf("Wrong PIN! %d attempt(s) left. Try again.\n", attempts);
            else
                printf("Wrong PIN! No attempts left. Cash Out cancelled.\n");
        }
    }
}

// ---------------- MOBILE RECHARGE ----------------
void mblRecharge(struct User *user) {
    char num[20];
    char operatorName[15];
    char pin[50];
    double amount;
    int conntype;
    char prefix[4];
    int attempts = 3;

    showOptionHeader();
    printf("==================[ MOBILE RECHARGE ]==================\n\n");

    do {
        printf("Enter number: ");
        scanf(" %19[^\n]", num);
    } while (!validatePhoneNumber(num));

    // Operator Detection based on Prefix
    strncpy(prefix, num, 3);
    prefix[3] = '\0';

    if (strcmp(prefix, "017") == 0 || strcmp(prefix, "013") == 0)
        strcpy(operatorName, "Grameenphone");
    else if (strcmp(prefix, "019") == 0 || strcmp(prefix, "014") == 0)
        strcpy(operatorName, "Banglalink");
    else if (strcmp(prefix, "018") == 0 || strcmp(prefix, "016") == 0)
        strcpy(operatorName, "Robi");
    else if (strcmp(prefix, "015") == 0)
        strcpy(operatorName, "Teletalk");
    else
        strcpy(operatorName, "Unknown");

    showOptionHeader();
    printf("==================[ MOBILE RECHARGE ]==================\n\n");
    printf("Recipient number: %s\n", num);
    printf("Operator name   : %s\n\n", operatorName);
    printf("Enter Recharge Amount: ");
    scanf("%lf", &amount);

    if (amount <= 10) {
        printf("Sorry! Minimum recharge amount is 11 Taka.\n");
        return;
    }
    if (amount > user->balance) {
        printf("Insufficient Balance! You have %.2f Taka.\n", user->balance);
        return;
    }

    showOptionHeader();
    printf("==================[ MOBILE RECHARGE ]==================\n\n");
    printf("Select Type:\n");
    printf("   1. Prepaid\n");
    printf("   2. Postpaid\n");
    printf("Choice: ");
    scanf("%d", &conntype);

    if (conntype != 1 && conntype != 2) {
        printf("Invalid choice.\n");
        return;
    }

    showOptionHeader();
    printf("==================[ MOBILE RECHARGE ]==================\n\n");

    while (attempts > 0) {
        printf("Enter PIN: ");
        scanf("%49s", pin);

        if (strcmp(pin, user->password) == 0) {
            user->balance -= amount;

            if (updateUserInFile(user)) {
                showOptionHeader();
                printf("==================[ MOBILE RECHARGE ]==================\n\n");
                printf("SUCCESS! Recharge of %.2f BDT sent to %s (%s).\n", amount, num, operatorName);
                printf("Current Balance: %.2f BDT\n", user->balance);
            } else {
                user->balance += amount; // Rollback
                printf("Transaction Failed! Balance not updated.\n");
            }
            return;
        } else {
            attempts--;
            if (attempts > 0)
                printf("Wrong PIN! %d attempt(s) left. Try again.\n", attempts);
            else
                printf("Wrong PIN! Mobile Recharge cancelled.\n");
        }
    }
}

// ---------------- SENDER: SEND MONEY WITH OTP ----------------
void sendMoney(struct User *sender) {
    char receiverPhone[50];
    char password[50];
    double amount;
    int attempts = 3;
    struct User receiver;
    FILE *fp;
    int found = 0;

    printf("\n~~~~~~~~~~>> SEND MONEY (OTP CLAIM) <<~~~~~~~~~~\n");

    do {
        printf("Enter Receiver Phone Number: ");
        scanf("%49s", receiverPhone);
    } while (!validatePhoneNumber(receiverPhone));

    if (strcmp(sender->phone, receiverPhone) == 0) {
        printf("Error: You cannot send money to your own number!\n");
        return;
    }

    fp = fopen(FILENAME, "rb");
    if (fp != NULL) {
        while (fread(&receiver, sizeof(struct User), 1, fp) == 1) {
            if (strcmp(receiver.phone, receiverPhone) == 0) {
                found = 1;
                break;
            }
        }
        fclose(fp);
    }

    if (!found) {
        printf("Error: Receiver phone number not registered in PocketFlow!\n");
        return;
    }

    printf("Enter Amount to Send: ");
    scanf("%lf", &amount);

    if (amount <= 0) {
        printf("Invalid Amount! Amount must be greater than 0.\n");
        return;
    }

    if (amount > sender->balance) {
        printf("Insufficient Balance! You have %.2f Taka.\n", sender->balance);
        return;
    }

    while (attempts > 0) {
        printf("Enter your PIN/Password to confirm: ");
        scanf("%49s", password);

        if (strcmp(password, sender->password) == 0) {
            
            srand(time(NULL));
            int generatedOTP = (rand() % 9000) + 1000; // 4-digit OTP

            sender->balance -= amount;
            if (updateUserInFile(sender)) {
                
                FILE *cfp = fopen(CLAIMS_FILE, "ab");
                if (cfp != NULL) {
                    struct Claim newClaim;
                    strcpy(newClaim.senderPhone, sender->phone);
                    strcpy(newClaim.receiverPhone, receiverPhone);
                    newClaim.amount = amount;
                    newClaim.otp = generatedOTP;
                    newClaim.status = 0; // Pending

                    fwrite(&newClaim, sizeof(struct Claim), 1, cfp);
                    fclose(cfp);
                }

                showOptionHeader();
                printf("\nSUCCESS! %.2f BDT sent to %s (Pending Claim)\n", amount, receiverPhone);
                printf("=========================================\n");
                printf("  SECURITY OTP CODE : [ %d ]             \n", generatedOTP);
                printf("=========================================\n");
                printf("Share this 4-digit OTP with the receiver to claim the money.\n");
                printf("Current Balance: %.2f BDT\n", sender->balance);

            } else {
                sender->balance += amount; // Rollback
                printf("Transaction Failed! System error.\n");
            }
            return;
        } else {
            attempts--;
            if (attempts > 0)
                printf("Wrong PIN! %d attempt(s) left.\n", attempts);
            else
                printf("Wrong PIN! Send Money cancelled.\n");
        }
    }
}

// Receiver's Notification Count
int getPendingNotificationCount(char *userPhone) {
    FILE *cfp = fopen(CLAIMS_FILE, "rb");
    if (cfp == NULL) return 0;

    struct Claim claim;
    int count = 0;

    while (fread(&claim, sizeof(struct Claim), 1, cfp) == 1) {
        if (strcmp(claim.receiverPhone, userPhone) == 0 && claim.status == 0) {
            count++;
        }
        if (strcmp(claim.senderPhone, userPhone) == 0 && claim.status == 2) {
            count++;
        }
    }

    fclose(cfp);
    return count;
}

// Auto Refund for wrong otp 3 times
void refundToSender(char *senderPhone, double amount) {
    FILE *fp = fopen(FILENAME, "r+b");
    if (fp == NULL) return;

    struct User temp;
    while (fread(&temp, sizeof(struct User), 1, fp) == 1) {
        if (strcmp(temp.phone, senderPhone) == 0) {
            temp.balance += amount;
            fseek(fp, -(long)sizeof(struct User), SEEK_CUR);
            fwrite(&temp, sizeof(struct User), 1, fp);
            break;
        }
    }
    fclose(fp);
}

// ---------------- NOTIFICATION & CLAIM SYSTEM ----------------
void showNotifications(struct User *currentUser) {
    FILE *cfp = fopen(CLAIMS_FILE, "r+b");
    if (cfp == NULL) {
        printf("\nNotifications: You have no new notifications.\n");
        return;
    }

    struct Claim claim;
    int found = 0;
    int inputOTP;
    int attempts;
    long recordPosition;

    printf("\n================ NOTIFICATION CENTER ================\n");

    while (fread(&claim, sizeof(struct Claim), 1, cfp) == 1) {
        recordPosition = ftell(cfp) - sizeof(struct Claim);

        // 1. New Transfer Pending
        if (strcmp(claim.receiverPhone, currentUser->phone) == 0 && claim.status == 0) {
            found = 1;
            printf("\nNEW TRANSFER RECEIVED!\n");
            printf("Sender  : %s\n", claim.senderPhone);
            printf("Amount  : %.2f BDT\n", claim.amount);
            printf("---------------------------------------------------\n");

            printf("1. Claim Received Money (Enter OTP)\n");
            printf("2. Skip / Back to Dashboard\n");
            printf("Choose option: ");
            int opt;
            scanf("%d", &opt);

            if (opt == 1) {
                attempts = 3;

                while (attempts > 0) {
                    printf("\nEnter 4-digit OTP code: ");
                    scanf("%d", &inputOTP);

                    if (inputOTP == claim.otp) {
                        currentUser->balance += claim.amount;
                        if (updateUserInFile(currentUser)) {
                            claim.status = 1; // Mark as Claimed
                            fseek(cfp, recordPosition, SEEK_SET);
                            fwrite(&claim, sizeof(struct Claim), 1, cfp);

                            printf("\nSUCCESS! %.2f BDT added to your account.\n", claim.amount);
                            printf("Current Balance: %.2f BDT\n", currentUser->balance);
                        } else {
                            currentUser->balance -= claim.amount; // Rollback
                            printf("Error updating balance.\n");
                        }
                        break;
                    } else {
                        attempts--;
                        if (attempts > 0) {
                            printf("Invalid OTP! Attempt(s) left: %d. Try again.\n", attempts);
                        } else {
                            printf("\nFailed! 3 Wrong Attempts. Transfer Unsuccessful.\n");
                            
                            refundToSender(claim.senderPhone, claim.amount);
                            
                            claim.status = 2; // Mark as Failed/Refunded
                            fseek(cfp, recordPosition, SEEK_SET);
                            fwrite(&claim, sizeof(struct Claim), 1, cfp);

                            printf("Money has been automatically refunded back to sender (%s).\n", claim.senderPhone);
                        }
                    }
                }
            }
            break;
        }

        // 2. Refund Alert for Sender
        if (strcmp(claim.senderPhone, currentUser->phone) == 0 && claim.status == 2) {
            found = 1;
            printf("\nUNSUCCESSFUL TRANSFER ALERT!\n");
            printf("Your transfer of %.2f BDT to %s failed due to 3 wrong OTP attempts.\n", claim.amount, claim.receiverPhone);
            printf("STATUS: The amount has been REFUNDED back to your account.\n");
            printf("---------------------------------------------------\n");

            claim.status = 3; // Archive
            fseek(cfp, recordPosition, SEEK_SET);
            fwrite(&claim, sizeof(struct Claim), 1, cfp);
            
            currentUser->balance += claim.amount;
            break;
        }
    }

    if (!found) {
        printf("\nYou have no new notifications.\n");
    }

    fclose(cfp);
}

// ---------------- DASHBOARD ----------------
void dashboard(struct User *user) {
    int choice;

    while (1) {
        showDashboardHeader(user->balance);

        int notifCount = getPendingNotificationCount(user->phone);

        printf("1. Add Money\n");
        printf("2. Cash Out\n");
        printf("3. Mobile Recharge\n");
        printf("4. Send Money (OTP Transfer)\n");
        if (notifCount > 0) {
            printf("5. Notification (%d) [NEW!]\n", notifCount);
        } else {
            printf("5. Notification (0)\n");
        }
        printf("6. Transaction History\n");
        printf("7. Logout\n");
        printf("Choose an option: ");
        scanf("%d", &choice);

        switch (choice) {
        case 1:
            showOptionHeader();
            addMoney(user);
            printf("\nPress Enter to return to dashboard...");
            getchar(); getchar();
            break;

        case 2:
            showOptionHeader();
            cashOut(user);
            printf("\nPress Enter to return to dashboard...");
            getchar(); getchar();
            break;

        case 3:
            mblRecharge(user);
            printf("\nPress Enter to return to dashboard...");
            getchar(); getchar();
            break;

        case 4:
            showOptionHeader();
            sendMoney(user);
            printf("\nPress Enter to return to dashboard...");
            getchar(); getchar();
            break;

        case 5:
            showOptionHeader();
            showNotifications(user);
            printf("\nPress Enter to return to dashboard...");
            getchar(); getchar();
            break;

        case 6:
            showOptionHeader();
            printf("--- Transaction History ---\n");
            printf("\nPress Enter to return to dashboard...");
            getchar(); getchar();
            break;

        case 7:
            return;

        default:
            printf("Invalid choice.\n");
        }
    }
}

// ---------------- MAIN MENU ----------------
int main() {
    int choice;
    struct User currentUser;

    while (1) {
        clearScreenAndShowBanner();
        printf("1. Sign Up\n");
        printf("2. Sign In\n");
        printf("3. Exit\n");
        printf("Choose an option: ");
        scanf("%d", &choice);

        switch (choice) {
        case 1:
            signUp();
            break;
        case 2:
            if (signIn(&currentUser) == 1) {
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