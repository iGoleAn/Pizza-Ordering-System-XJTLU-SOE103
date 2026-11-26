#include "input.h"
#include "order_port.h"
#include "../E/sales.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#define MANAGER_NAME "manager"
#define MANAGER_PASSWORD "Pizza103"

static int loginManager(void)
{
    char name[40], password[40];

    puts("\nManager sign-in: enter your shop manager username and password.");
    if (promptLine("Manager username: ", name, sizeof name) != 1 ||
        promptLine("Manager password: ", password, sizeof password) != 1)
        return 0;
    if (strcmp(name, MANAGER_NAME) == 0 && strcmp(password, MANAGER_PASSWORD) == 0)
        return 1;
    puts("Manager login failed. Choose Manager login to try again.");
    return 0;
}

static int loginCustomer(const CustomerList *customers)
{
    char phone[CUST_PHONE_LEN];
    const Customer *customer;
    int id;

    puts("\nNew customer? Return to the main menu and select 2 to register.");
    puts("To sign in, use the account number issued at registration and the same phone number.");
    if (promptInt("Account number (0 to return): ", 0, INT_MAX - 1, &id) != 1 || id == 0)
        return 0;
    if (id < CUST_START_ID) {
        printf("Customer account numbers start at %d. Check the number and try again.\n",
               CUST_START_ID);
        return 0;
    }
    if (promptLine("Registered phone number: ", phone, sizeof phone) != 1)
        return 0;
    customer = customerFindByIdConst(customers, id);
    if (customer != NULL && strcmp(customer->telephone, phone) == 0)
        return id;
    puts("Account number or phone number does not match. Check both and try again.");
    return 0;
}

static int saveAccounts(const CustomerList *customers)
{
    if (customerSave(customers, CUST_DATA_FILE) == CUST_OK)
        return 1;
    puts("Could not save customers.dat. The program will retry on exit.");
    return 0;
}

static int saveMenu(const PizzaMenu *menu)
{
    if (menu_save(menu, MENU_DATA_FILE))
        return 1;
    puts("Could not save pizza_prices.txt. The program will retry on exit.");
    return 0;
}

static void customerMenu(const PizzaMenu *menu, CustomerList *customers,
                         int accountId, int *accountsUnsaved)
{
    int choice, result;

    printf("\nSigned in as %s (account %d).\n",
           customerFindByIdConst(customers, accountId)->name, accountId);
    for (;;) {
        puts("\n1. View pizza menu\n2. Check balance\n3. Deposit money");
#ifdef WITH_ORDERS
        puts("4. Place an order\n5. Review your completed orders");
#else
        puts("4. Build a pizza and preview its price\n5. Order history (currently unavailable)");
#endif
        puts("0. Log out");
        result = promptInt("Choice: ", 0, 5, &choice);
        if (result == -1 || (result == 1 && choice == 0))
            return;
        if (result == 0)
            continue;
        switch (choice) {
        case 1:
            menu_display(menu);
            break;
        case 2:
            printf("Available balance: $%.2f\n", customerGetBalance(customers, accountId));
            break;
        case 3:
            if (customerTopUpPrompt(customers, accountId))
                *accountsUnsaved = !saveAccounts(customers);
            break;
        case 4:
#ifdef WITH_ORDERS
            if (*accountsUnsaved)
                *accountsUnsaved = !saveAccounts(customers);
            if (!*accountsUnsaved) {
                printf("Available balance: $%.2f\n",
                       customerGetBalance(customers, accountId));
                orderPlace(menu, customers, accountId);
            } else {
                puts("Ordering is unavailable until the account data can be saved.");
            }
#else
            menu_customer_interface(menu);
#endif
            break;
        case 5:
#ifdef WITH_ORDERS
            orderShowHistory(accountId);
#else
            puts("Order history is currently unavailable. Choose 4 to preview a pizza price.");
#endif
            break;
        }
    }
}

static void managerMenu(PizzaMenu *menu, CustomerList *customers,
                        int *menuUnsaved, int *accountsUnsaved)
{
    int choice, result;

    for (;;) {
        puts("\n========== MANAGER ==========");
        puts("1. Edit pizza options and prices");
        puts("2. Add, edit, remove or list customer accounts");
        puts("3. View sales totals for completed orders\n0. Log out");
        result = promptInt("Choice: ", 0, 3, &choice);
        if (result == -1 || (result == 1 && choice == 0))
            return;
        if (result == 0)
            continue;
        switch (choice) {
        case 1:
            if (menu_manager_interface(menu)) {
                *menuUnsaved = !saveMenu(menu);
                if (!*menuUnsaved)
                    puts("Menu updated and saved.");
            }
            break;
        case 2:
            if (customerManagerMenu(customers))
                *accountsUnsaved = !saveAccounts(customers);
            break;
        case 3:
            salesDisplay(ORDER_DATA_FILE);
            break;
        }
    }
}

int main(void)
{
    PizzaMenu menu;
    CustomerList customers;
    int choice, result, id;
    int accountsUnsaved = 0, menuUnsaved = 0;

    menu_init(&menu);
    customerListInit(&customers);
    if (!menu_load_if_present(&menu, MENU_DATA_FILE)) {
        puts("Cannot read pizza_prices.txt. Check the file before restarting.");
        return EXIT_FAILURE;
    }
    if (customerLoad(&customers, CUST_DATA_FILE) != CUST_OK) {
        puts("Cannot read customers.dat. Check the file before restarting.");
        customerListFree(&customers);
        return EXIT_FAILURE;
    }

    for (;;) {
        puts("\n========== PIZZA SHOP ==========");
        puts("1. Customer login\n2. Register customer account\n3. Manager login\n0. Exit");
        result = promptInt("Choice: ", 0, 3, &choice);
        if (result == -1 || (result == 1 && choice == 0))
            break;
        if (result == 0)
            continue;
        switch (choice) {
        case 1:
            id = loginCustomer(&customers);
            if (id > 0)
                customerMenu(&menu, &customers, id, &accountsUnsaved);
            break;
        case 2:
            id = customerRegisterPrompt(&customers);
            if (id > 0) {
                printf("Your account number is %d.\n", id);
                puts("Keep this number. To sign in, choose 1 and use it with your registered phone number.");
                accountsUnsaved = !saveAccounts(&customers);
            }
            break;
        case 3:
            if (loginManager())
                managerMenu(&menu, &customers, &menuUnsaved, &accountsUnsaved);
            break;
        }
    }

    if (accountsUnsaved)
        accountsUnsaved = !saveAccounts(&customers);
    if (menuUnsaved)
        menuUnsaved = !saveMenu(&menu);
    customerListFree(&customers);
    if (accountsUnsaved || menuUnsaved) {
        puts("Some changes could not be saved.");
        return EXIT_FAILURE;
    }
    puts("Goodbye.");
    return EXIT_SUCCESS;
}
