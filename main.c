void addMoney(int userIndex){
    char agentNum[12];
    double amount;
    printf("\n~~~~~~~~~~>>ADD MONEY<<~~~~~~~~~~\n");
    printf("Enter Agent Number: ");
    scanf("%[^\n]",agentNum);

    //validation Function - Riyad

    printf("Enter Amount to Add: ");
    scanf("%lf", &amount);

    if(amount <= 0){
        printf("Your amount is too low. Try again");
        return;
    }

    //balace += amount;- Riyad

}