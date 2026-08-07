#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <time.h> 
#include <ctype.h>

#define FILENAME "users.dat"
#define CLAIMS_FILE "claims.dat"
#define TXN_FILE "transactions.dat"

#define CASH_OUT_RATE 0.015

// ---------------- DATA STRUCTURES ----------------

struct User {
    char name[100];
    char phone[50];
    char password[50];
    double balance;
};

struct Claim {
    char senderPhone[50];
    char receiverPhone[50];
    double amount;
    int otp;
    int status; // 0 = Pending, 1 = Claimed, 2 = Failed/Refunded, 3 = Archived
};

struct Transaction {
    char userPhone[50];
    char type[30];    
    char details[50]; 
    double amount;
    double postBalance;
    char timestamp[20];
};

// ---------------- UI & SCREEN HELPERS ----------------

void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void clearBuffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

void pauseScreen() {
    printf("\n  Press Enter to continue...");
    fflush(stdout);
    clearBuffer();
}

void renderBanner() {
    clearScreen();
    printf("\n  +===================================================================+\n");
    printf("  |                         P O C K E T F L O W                       |\n");
    printf("  |                  Next-Gen Digital Wallet & Finance                |\n");
    printf("  +===================================================================+\n\n");
}

void renderPageHeader(const char *title, const struct User *user) {
    renderBanner();
    if (user != NULL) {
        printf("  Account: %-28s Balance: %10.2f BDT\n", user->name, user->balance);
        printf("  Phone  : %-28s\n", user->phone);
        printf("  -------------------------------------------------------------------\n");
    }
    printf("  >>> %s <<<\n", title);
    printf("  -------------------------------------------------------------------\n\n");
}

// ---------------- UTILITIES & VALIDATION ----------------

void getCurrentFormattedTime(char *buffer) {
    time_t rawtime;
    time(&rawtime);
    strftime(buffer, 20, "%Y-%m-%d %H:%M", localtime(&rawtime));
}

int validatePhoneNumber(const char *number) {
    if (strlen(number) != 11) {
        printf("  [!] Invalid length. Phone number must be exactly 11 digits.\n");
        return 0;
    }
    for (int i = 0; i < 11; i++) {
        if (!isdigit(number[i])) {
            printf("  [!] Invalid format. Number must contain digits only.\n");
            return 0;
        }
    }
    if (number[0] != '0' || number[1] != '1' || number[2] < '3' || number[2] > '9') {
        printf("  [!] Invalid prefix. Number must start with 013-019.\n");
        return 0;
    }
    return 1;
}

const char* getOperatorName(const char *phone) {
    if (strncmp(phone, "017", 3) == 0 || strncmp(phone, "013", 3) == 0) return "Grameenphone";
    if (strncmp(phone, "019", 3) == 0 || strncmp(phone, "014", 3) == 0) return "Banglalink";
    if (strncmp(phone, "018", 3) == 0 || strncmp(phone, "016", 3) == 0) return "Robi";
    if (strncmp(phone, "015", 3) == 0) return "Teletalk";
    return "Unknown Operator";
}

int verifyUserPin(const struct User *user) {
    char inputPin[50];
    for (int attempts = 3; attempts > 0; attempts--) {
        printf("  Enter Security PIN: ");
        scanf("%49s", inputPin);
        clearBuffer();

        if (strcmp(inputPin, user->password) == 0) return 1;

        if (attempts > 1) {
            printf("  [!] Incorrect PIN. %d attempt(s) remaining.\n\n", attempts - 1);
        } else {
            printf("  [!] Incorrect PIN. Action canceled for security.\n");
        }
    }
    return 0;
}

void getValidPhoneInput(const char *prompt, char *output) {
    do {
        printf("  %s", prompt);
        scanf("%49s", output);
        clearBuffer();
    } while (!validatePhoneNumber(output));
}

double getValidAmountInput(const char *prompt, double minAmount) {
    double amount;
    printf("  %s", prompt);
    if (scanf("%lf", &amount) != 1 || amount < minAmount) {
        clearBuffer();
        printf("  [!] Invalid amount (Minimum requirement: %.2f BDT).\n", minAmount);
        return -1.0;
    }
    clearBuffer();
    return amount;
}

// ---------------- FILE DATA OPERATIONS ----------------

int updateUserInFile(struct User *user) {
    FILE *fp = fopen(FILENAME, "r+b");
    if (!fp) return 0;

    struct User temp;
    while (fread(&temp, sizeof(struct User), 1, fp) == 1) {
        if (strcmp(temp.phone, user->phone) == 0) {
            fseek(fp, -(long)sizeof(struct User), SEEK_CUR);
            fwrite(user, sizeof(struct User), 1, fp);
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    return 0;
}

void recordTransaction(const char *userPhone, const char *type, const char *details, double amount, double postBalance) {
    FILE *fp = fopen(TXN_FILE, "ab");
    if (!fp) return;

    struct Transaction txn;
    memset(&txn, 0, sizeof(struct Transaction));
    strncpy(txn.userPhone, userPhone, sizeof(txn.userPhone) - 1);
    strncpy(txn.type, type, sizeof(txn.type) - 1);
    strncpy(txn.details, details, sizeof(txn.details) - 1);
    txn.amount = amount;
    txn.postBalance = postBalance;
    getCurrentFormattedTime(txn.timestamp);

    fwrite(&txn, sizeof(struct Transaction), 1, fp);
    fclose(fp);
}

int getPendingNotificationCount(const char *userPhone) {
    FILE *cfp = fopen(CLAIMS_FILE, "rb");
    if (!cfp) return 0;

    struct Claim claim;
    int count = 0;
    while (fread(&claim, sizeof(struct Claim), 1, cfp) == 1) {
        if ((strcmp(claim.receiverPhone, userPhone) == 0 && claim.status == 0) ||
            (strcmp(claim.senderPhone, userPhone) == 0 && claim.status == 2)) {
            count++;
        }
    }
    fclose(cfp);
    return count;
}

void refundToSender(const char *senderPhone, double amount) {
    FILE *fp = fopen(FILENAME, "r+b");
    if (!fp) return;

    struct User temp;
    while (fread(&temp, sizeof(struct User), 1, fp) == 1) {
        if (strcmp(temp.phone, senderPhone) == 0) {
            temp.balance += amount;
            fseek(fp, -(long)sizeof(struct User), SEEK_CUR);
            fwrite(&temp, sizeof(struct User), 1, fp);
            recordTransaction(temp.phone, "Refund Received", "Auto-Refund", amount, temp.balance);
            break;
        }
    }
    fclose(fp);
}

int findUserByPhone(const char *phone, struct User *outUser) {
    FILE *fp = fopen(FILENAME, "rb");
    if (!fp) return 0;

    struct User temp;
    while (fread(&temp, sizeof(struct User), 1, fp) == 1) {
        if (strcmp(temp.phone, phone) == 0) {
            if (outUser) *outUser = temp;
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    return 0;
}

// ---------------- USER AUTHENTICATION ----------------

void signUp() {
    struct User newUser;
    memset(&newUser, 0, sizeof(struct User));

    renderPageHeader("USER REGISTRATION", NULL);

    int validName;
    do {
        validName = 1;
        printf("  Enter Full Name: ");
        scanf(" %99[^\n]", newUser.name);
        clearBuffer();

        for (int i = 0; newUser.name[i] != '\0'; i++) {
            if (!isalpha(newUser.name[i]) && !isspace(newUser.name[i])) {
                printf("  [!] Invalid Name. Alphabets and spaces only.\n");
                validName = 0;
                break;
            }
        }
    } while (!validName);

    getValidPhoneInput("Enter Phone Number (11 Digits): ", newUser.phone);

    if (findUserByPhone(newUser.phone, NULL)) {
        printf("\n  [!] Account already exists with this phone number!\n");
        pauseScreen();
        return;
    }

    int validPin = 0;
    do {
        printf("  Set 4-Digit Security PIN: ");
        scanf("%49s", newUser.password);
        clearBuffer();

        if (strlen(newUser.password) == 4) {
            validPin = 1;
            for (int i = 0; i < 4; i++) {
                if (!isdigit(newUser.password[i])) { validPin = 0; break; }
            }
        }
        if (!validPin) printf("  [!] PIN must consist of exactly 4 numeric digits.\n");
    } while (!validPin);

    FILE *fp = fopen(FILENAME, "ab");
    if (!fp) {
        printf("  [!] Unable to create account. Storage file write error.\n");
        pauseScreen();
        return;
    }
    fwrite(&newUser, sizeof(struct User), 1, fp);
    fclose(fp);

    printf("\n  [+] Account successfully registered! Welcome, %s.\n", newUser.name);
    pauseScreen();
}

int signIn(struct User *loggedInUser) {
    char phone[50], password[50];
    renderPageHeader("ACCOUNT SIGN IN", NULL);
    
    printf("  Enter Phone Number: ");
    scanf("%49s", phone);
    clearBuffer();

    if (!findUserByPhone(phone, loggedInUser)) {
        printf("\n  [!] Account not found. Please sign up first.\n");
        pauseScreen();
        return 0;
    }

    for (int attempts = 3; attempts > 0; attempts--) {
        printf("  Enter Security PIN: ");
        scanf("%49s", password);
        clearBuffer();

        if (strcmp(loggedInUser->password, password) == 0) {
            printf("\n  [+] Login successful! Welcome back, %s.\n", loggedInUser->name);
            sleep(1);
            return 1;
        }

        if (attempts > 1) printf("  [!] Wrong PIN! %d attempt(s) remaining.\n\n", attempts - 1);
        else printf("\n  [!] Too many incorrect attempts. Login failed.\n");
    }

    pauseScreen();
    return 0;
}

// ---------------- FINANCIAL SERVICES ----------------

void addMoney(struct User *user) {
    char agentNum[50];
    renderPageHeader("ADD MONEY", user);

    getValidPhoneInput("Enter Agent Phone Number: ", agentNum);
    double amount = getValidAmountInput("Enter Amount (BDT): ", 1.0);
    if (amount <= 0) { pauseScreen(); return; }

    user->balance += amount;
    if (updateUserInFile(user)) {
        recordTransaction(user->phone, "Add Money", agentNum, amount, user->balance);
        printf("\n  [+] Successfully added %.2f BDT from Agent %s.\n", amount, agentNum);
        printf("  [+] Updated Balance: %.2f BDT\n", user->balance);
    } else {
        user->balance -= amount;
        printf("\n  [!] System error. Balance update failed.\n");
    }
    pauseScreen();
}

void cashOut(struct User *user) {
    char agentNum[50];
    renderPageHeader("CASH OUT", user);

    getValidPhoneInput("Enter Agent Phone Number: ", agentNum);
    double amount = getValidAmountInput("Enter Amount to Cash Out: ", 1.0);
    if (amount <= 0) { pauseScreen(); return; }

    double charge = amount * CASH_OUT_RATE;
    double totalDeduction = amount + charge;

    if (totalDeduction > user->balance) {
        printf("  [!] Insufficient Balance! Total required (inc. 1.5%% fee): %.2f BDT\n", totalDeduction);
        pauseScreen();
        return;
    }

    printf("\n  Summary: Cash Out Amount: %.2f BDT | Fee: %.2f BDT | Total: %.2f BDT\n\n", amount, charge, totalDeduction);

    if (verifyUserPin(user)) {
        user->balance -= totalDeduction;
        if (updateUserInFile(user)) {
            recordTransaction(user->phone, "Cash Out", agentNum, totalDeduction, user->balance);
            printf("\n  [+] Successfully cashed out %.2f BDT via Agent %s!\n", amount, agentNum);
            printf("  [+] Remaining Balance: %.2f BDT\n", user->balance);
        } else {
            user->balance += totalDeduction;
            printf("\n  [!] System failure. Balance unchanged.\n");
        }
    }
    pauseScreen();
}

void mblRecharge(struct User *user) {
    char num[50];
    int connType;
    renderPageHeader("MOBILE RECHARGE", user);

    getValidPhoneInput("Enter Target Mobile Number: ", num);
    const char *operatorName = getOperatorName(num);
    printf("  Detected Operator: %s\n\n", operatorName);

    double amount = getValidAmountInput("Enter Recharge Amount (Min 11 BDT): ", 11.0);
    if (amount <= 0) { pauseScreen(); return; }

    if (amount > user->balance) {
        printf("  [!] Insufficient balance. Available: %.2f BDT\n", user->balance);
        pauseScreen();
        return;
    }

    printf("\n  Select Connection Type:\n  1. Prepaid\n  2. Postpaid\n  Choice (1-2): ");
    if (scanf("%d", &connType) != 1 || (connType != 1 && connType != 2)) {
        clearBuffer();
        printf("  [!] Invalid connection selection.\n");
        pauseScreen();
        return;
    }
    clearBuffer();

    printf("\n");
    if (verifyUserPin(user)) {
        user->balance -= amount;
        if (updateUserInFile(user)) {
            char detailStr[50];
            snprintf(detailStr, sizeof(detailStr), "%s (%s)", num, operatorName);
            recordTransaction(user->phone, "Mobile Recharge", detailStr, amount, user->balance);

            printf("\n  [+] Recharge of %.2f BDT successful to %s (%s)!\n", amount, num, operatorName);
            printf("  [+] Remaining Balance: %.2f BDT\n", user->balance);
        } else {
            user->balance += amount;
            printf("\n  [!] Transaction failed due to file update error.\n");
        }
    }
    pauseScreen();
}

void sendMoney(struct User *sender) {
    char receiverPhone[50];
    struct User receiver;

    renderPageHeader("SEND MONEY (OTP PROTECTED)", sender);
    getValidPhoneInput("Enter Receiver Phone Number: ", receiverPhone);

    if (strcmp(sender->phone, receiverPhone) == 0) {
        printf("  [!] You cannot transfer money to your own number.\n");
        pauseScreen();
        return;
    }

    if (!findUserByPhone(receiverPhone, &receiver)) {
        printf("  [!] Receiver account not registered on PocketFlow.\n");
        pauseScreen();
        return;
    }

    double amount = getValidAmountInput("Enter Amount to Send: ", 1.0);
    if (amount <= 0) { pauseScreen(); return; }

    if (amount > sender->balance) {
        printf("  [!] Insufficient balance! Available: %.2f BDT\n", sender->balance);
        pauseScreen();
        return;
    }

    printf("\n");
    if (verifyUserPin(sender)) {
        srand((unsigned int)time(NULL));
        int generatedOTP = (rand() % 9000) + 1000;

        sender->balance -= amount;
        if (updateUserInFile(sender)) {
            FILE *cfp = fopen(CLAIMS_FILE, "ab");
            if (cfp) {
                struct Claim newClaim = {0};
                strncpy(newClaim.senderPhone, sender->phone, sizeof(newClaim.senderPhone) - 1);
                strncpy(newClaim.receiverPhone, receiverPhone, sizeof(newClaim.receiverPhone) - 1);
                newClaim.amount = amount;
                newClaim.otp = generatedOTP;
                newClaim.status = 0;
                fwrite(&newClaim, sizeof(struct Claim), 1, cfp);
                fclose(cfp);
            }

            recordTransaction(sender->phone, "Send Money", receiverPhone, amount, sender->balance);

            printf("\n  +===================================================+\n");
            printf("  | TRANSFER INITIATED: Pending Receiver Claim        |\n");
            printf("  | Receiver : %-38s |\n", receiver.name);
            printf("  | Amount   : %-10.2f BDT                            |\n", amount);
            printf("  | SECURITY CLAIM OTP: [ %d ]                         |\n", generatedOTP);
            printf("  +===================================================+\n");
            printf("  Provide this 4-digit OTP to the receiver to claim the funds.\n");
        } else {
            sender->balance += amount;
            printf("  [!] Transaction system error.\n");
        }
    }
    pauseScreen();
}

// ---------------- NOTIFICATIONS & CLAIMS ----------------

void showNotifications(struct User *currentUser) {
    FILE *cfp = fopen(CLAIMS_FILE, "r+b");
    renderPageHeader("NOTIFICATION CENTER", currentUser);

    if (!cfp) {
        printf("  No new notifications found.\n");
        pauseScreen();
        return;
    }

    struct Claim claim;
    int found = 0;

    while (fread(&claim, sizeof(struct Claim), 1, cfp) == 1) {
        long recordPosition = ftell(cfp) - sizeof(struct Claim);

        if (strcmp(claim.receiverPhone, currentUser->phone) == 0 && claim.status == 0) {
            found = 1;
            printf("  [!] PENDING INCOMING TRANSFER DETECTED\n");
            printf("  From Sender : %s\n", claim.senderPhone);
            printf("  Amount      : %.2f BDT\n", claim.amount);
            printf("  ---------------------------------------------------\n");
            printf("  1. Claim Money (Enter Security OTP)\n");
            printf("  2. Skip for Later\n  Choice: ");

            int opt;
            if (scanf("%d", &opt) == 1 && opt == 1) {
                clearBuffer();
                int inputOTP, attempts = 3;

                while (attempts > 0) {
                    printf("  Enter 4-Digit Security OTP: ");
                    if (scanf("%d", &inputOTP) == 1 && inputOTP == claim.otp) {
                        clearBuffer();
                        currentUser->balance += claim.amount;
                        if (updateUserInFile(currentUser)) {
                            claim.status = 1;
                            fseek(cfp, recordPosition, SEEK_SET);
                            fwrite(&claim, sizeof(struct Claim), 1, cfp);
                            recordTransaction(currentUser->phone, "Received Money", claim.senderPhone, claim.amount, currentUser->balance);

                            printf("\n  [+] CLAIM SUCCESSFUL! %.2f BDT added to your account.\n", claim.amount);
                            printf("  [+] New Balance: %.2f BDT\n", currentUser->balance);
                        } else {
                            currentUser->balance -= claim.amount;
                            printf("  [!] Failed to update account record.\n");
                        }
                        break;
                    }
                    clearBuffer();
                    attempts--;
                    if (attempts > 0) printf("  [!] Incorrect OTP! %d attempt(s) remaining.\n", attempts);
                    else {
                        printf("\n  [!] 3 Failed OTP attempts. Transfer failed & automatically refunded.\n");
                        refundToSender(claim.senderPhone, claim.amount);
                        claim.status = 2;
                        fseek(cfp, recordPosition, SEEK_SET);
                        fwrite(&claim, sizeof(struct Claim), 1, cfp);
                    }
                }
            } else clearBuffer();
            break;
        }

        if (strcmp(claim.senderPhone, currentUser->phone) == 0 && claim.status == 2) {
            found = 1;
            printf("  [!] FAILED TRANSFER REFUND ALERT\n");
            printf("  Transfer of %.2f BDT to %s failed (Invalid OTPs).\n", claim.amount, claim.receiverPhone);
            printf("  STATUS: Amount has been automatically refunded to your wallet.\n");
            printf("  ---------------------------------------------------\n");

            claim.status = 3;
            fseek(cfp, recordPosition, SEEK_SET);
            fwrite(&claim, sizeof(struct Claim), 1, cfp);
            break;
        }
    }

    if (!found) printf("  No pending notifications at this time.\n");
    fclose(cfp);
    pauseScreen();
}

// ---------------- TRANSACTION HISTORY ----------------

void showTransactionHistory(const struct User *user) {
    FILE *fp = fopen(TXN_FILE, "rb");
    renderPageHeader("TRANSACTION HISTORY", user);

    if (!fp) {
        printf("  No transaction history records found.\n");
        pauseScreen();
        return;
    }

    struct Transaction txn;
    int count = 0;
    printf(" %-17s | %-16s | %-20s | %-10s | %-10s\n", "Date & Time", "Type", "Details / Target", "Amount", "Balance");
    printf(" -----------------------------------------------------------------------------------\n");

    while (fread(&txn, sizeof(struct Transaction), 1, fp) == 1) {
        if (strcmp(txn.userPhone, user->phone) == 0) {
            count++;
            char amtStr[20];
            if (strcmp(txn.type, "Add Money") == 0 || strcmp(txn.type, "Received Money") == 0 || strcmp(txn.type, "Refund Received") == 0) {
                snprintf(amtStr, sizeof(amtStr), "+%.2f", txn.amount);
            } else {
                snprintf(amtStr, sizeof(amtStr), "-%.2f", txn.amount);
            }
            printf(" %-17s | %-16s | %-20s | %-10s | %-10.2f\n", txn.timestamp, txn.type, txn.details, amtStr, txn.postBalance);
        }
    }

    if (count == 0) printf("                     No past transactions recorded for this user.\n");
    printf(" -----------------------------------------------------------------------------------\n");
    printf(" Total Recorded Transactions: %d\n", count);

    fclose(fp);
    pauseScreen();
}

// ---------------- DASHBOARD & NAVIGATION ----------------

void dashboard(struct User *user) {
    int choice;
    while (1) {
        renderPageHeader("ACCOUNT DASHBOARD", user);
        int notifCount = getPendingNotificationCount(user->phone);

        printf("  1. Add Money\n");
        printf("  2. Cash Out\n");
        printf("  3. Mobile Recharge\n");
        printf("  4. Send Money (OTP Transfer)\n");
        printf("  5. Notifications (%d)%s\n", notifCount, notifCount > 0 ? " [NEW!]" : "");
        printf("  6. Transaction History\n");
        printf("  7. Logout\n");
        printf("\n  Select Service (1-7): ");

        if (scanf("%d", &choice) != 1) { clearBuffer(); continue; }
        clearBuffer();

        switch (choice) {
            case 1: addMoney(user); break;
            case 2: cashOut(user); break;
            case 3: mblRecharge(user); break;
            case 4: sendMoney(user); break;
            case 5: showNotifications(user); break;
            case 6: showTransactionHistory(user); break;
            case 7: return;
            default: break;
        }
    }
}

// ---------------- MAIN ROUTINE ----------------

int main() {
    int choice;
    struct User currentUser;

    while (1) {
        renderBanner();
        printf("  1. Sign Up\n");
        printf("  2. Sign In\n");
        printf("  3. Exit\n");
        printf("\n  Choose Option (1-3): ");

        if (scanf("%d", &choice) != 1) { clearBuffer(); continue; }
        clearBuffer();

        switch (choice) {
            case 1: signUp(); break;
            case 2: if (signIn(&currentUser)) dashboard(&currentUser); break;
            case 3:
                renderBanner();
                printf("  Thank you for using PocketFlow. Goodbye!\n\n");
                return 0;
            default: break;
        }
    }
    return 0;
}