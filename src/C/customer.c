/*
 * ============================================================================
 * File        : customer.c
 * Module      : C - Customer Account System
 * Course      : SOE103 C Programming and Software Engineering
 * Project     : Pizza Ordering System (Group Project)
 * Description : Implementation of the customer account module.
 *
 * Functions provided:
 *   - register a customer with a unique account number
 *   - edit and delete accounts
 *   - enquire the current balance
 *   - top up (deposit) money
 *   - charge an account when an order is placed (used by order.c)
 *   - load / save the account database to a text file
 *   - a menu-driven interface for both manager and customer
 *
 * All public functions validate their arguments and return a status code.
 * The file is written in C89-compatible style so it can be compiled by any
 * standard C compiler used in the laboratory.
 * ============================================================================
 */

#include "customer.h"
#include "../A/input.h"
#include "../A/datafile.h"
#include <errno.h>

#include <stdio.h>
#include <stdlib.h>     /* malloc, realloc, free                              */
#include <string.h>
#include <ctype.h>      /* isdigit                                            */
#include <limits.h>     /* INT_MAX                                            */

#define MIN_PHONE_DIGITS 6

/* ---------------------------------------------------------------------------
 * Internal helper: copy a string safely into a fixed-size field.
 * Guarantees NUL-termination even when the source is longer than the field.
 * ------------------------------------------------------------------------ */
static void copyField(char *dest, int destSize, const char *src)
{
    int i;

    if (destSize <= 0) {
        return;
    }
    if (src == NULL) {
        dest[0] = '\0';
        return;
    }
    for (i = 0; i < destSize - 1 && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    dest[i] = '\0';
}

/* ---------------------------------------------------------------------------
 * Internal helper: validate a name.
 * A valid name contains at least one character and no '|' (the field
 * separator used by the data file) and no newline.
 * ------------------------------------------------------------------------ */
static int isValidName(const char *name)
{
    const char *p;

    if (name == NULL || strlen(name) >= CUST_NAME_LEN) {
        return 0;
    }
    for (p = name; *p != '\0'; p++) {
        if (*p == '|' || *p == '\n' || *p == '\r') {
            return 0;
        }
    }
    /* reject an all-blank name */
    for (p = name; *p != '\0'; p++) {
        if (!isspace((unsigned char)*p)) {
            return 1;
        }
    }
    return 0;
}

/* ---------------------------------------------------------------------------
 * Internal helper: validate a telephone number.
 * Accepts digits, spaces, '+' and '-', but requires at least 6 digits so a
 * single keystroke cannot be registered as a phone number.
 * ------------------------------------------------------------------------ */
static int isValidPhone(const char *phone)
{
    const char *p;
    int digits = 0;

    if (phone == NULL || strlen(phone) >= CUST_PHONE_LEN) {
        return 0;
    }
    for (p = phone; *p != '\0'; p++) {
        if (isdigit((unsigned char)*p)) {
            digits++;
        } else if (*p == ' ' || *p == '-' || *p == '+') {
            /* allowed formatting characters */
        } else {
            return 0;   /* reject '|', letters, control characters, ... */
        }
    }
    return digits >= MIN_PHONE_DIGITS;
}

/* ---------------------------------------------------------------------------
 * Internal helper: make sure there is room for one more record.
 * The array grows geometrically to keep insertion cheap.
 * Returns CUST_OK or CUST_ERR_NOMEM / CUST_ERR_INVALID.
 * ------------------------------------------------------------------------ */
static int ensureCapacity(CustomerList *list)
{
    Customer *tmp;
    int newCapacity;
    int i;

    if (list == NULL) {
        return CUST_ERR_INVALID;
    }
    if (list->count < list->capacity) {
        return CUST_OK;
    }
    if (list->capacity >= CUST_MAX_RECORDS) {
        return CUST_ERR_NOMEM;      /* configured safety limit reached */
    }

    newCapacity = (list->capacity == 0) ? 4 : list->capacity * 2;
    if (newCapacity > CUST_MAX_RECORDS) {
        newCapacity = CUST_MAX_RECORDS;
    }

    tmp = (Customer *)realloc(list->items, (size_t)newCapacity * sizeof(Customer));
    if (tmp == NULL) {
        return CUST_ERR_NOMEM;      /* original block is left untouched */
    }
    list->items = tmp;

    /* initialise the newly allocated slots */
    for (i = list->capacity; i < newCapacity; i++) {
        list->items[i].accountId = 0;
        list->items[i].name[0] = '\0';
        list->items[i].telephone[0] = '\0';
        list->items[i].balance = 0.0;
        list->items[i].orderCount = 0;
    }
    list->capacity = newCapacity;
    return CUST_OK;
}

/* ---------------------------------------------------------------------------
 * Life-cycle management
 * ------------------------------------------------------------------------ */

void customerListInit(CustomerList *list)
{
    if (list == NULL) {
        return;
    }
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
    list->nextId = CUST_START_ID;
}

void customerListFree(CustomerList *list)
{
    if (list == NULL) {
        return;
    }
    free(list->items);              /* free(NULL) is harmless */
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

/* ---------------------------------------------------------------------------
 * Core operations
 * ------------------------------------------------------------------------ */

int customerRegister(CustomerList *list, const char *name,
                     const char *telephone, double initialDeposit)
{
    int status;
    int id;

    if (list == NULL || !isValidName(name) || !isValidPhone(telephone)) {
        return CUST_ERR_INVALID;
    }
    if (!validMoney(initialDeposit, 0.0, CUST_MAX_BALANCE) ||
        list->nextId == INT_MAX) {
        return CUST_ERR_INVALID;
    }

    status = ensureCapacity(list);
    if (status != CUST_OK) {
        return status;
    }

    /* The next number is saved even when the latest account is deleted. */
    id = list->nextId++;

    list->items[list->count].accountId = id;
    copyField(list->items[list->count].name, CUST_NAME_LEN, name);
    copyField(list->items[list->count].telephone, CUST_PHONE_LEN, telephone);
    list->items[list->count].balance = moneyCents(initialDeposit) / 100.0;
    list->items[list->count].orderCount = 0;
    list->count++;

    return id;                      /* positive value = success */
}

int customerEdit(CustomerList *list, int accountId,
                 const char *newName, const char *newTelephone)
{
    Customer *c;

    if (list == NULL) {
        return CUST_ERR_INVALID;
    }
    if (newName != NULL && !isValidName(newName)) {
        return CUST_ERR_INVALID;
    }
    if (newTelephone != NULL && !isValidPhone(newTelephone)) {
        return CUST_ERR_INVALID;
    }
    if (newName == NULL && newTelephone == NULL) {
        return CUST_ERR_INVALID;    /* nothing to change */
    }

    c = customerFindById(list, accountId);
    if (c == NULL) {
        return CUST_ERR_NOTFOUND;
    }
    if (newName != NULL) {
        copyField(c->name, CUST_NAME_LEN, newName);
    }
    if (newTelephone != NULL) {
        copyField(c->telephone, CUST_PHONE_LEN, newTelephone);
    }
    return CUST_OK;
}

int customerDelete(CustomerList *list, int accountId)
{
    int i;

    if (list == NULL) {
        return CUST_ERR_INVALID;
    }
    for (i = 0; i < list->count; i++) {
        if (list->items[i].accountId == accountId) {
            /* Replace the removed slot with the last record.  Ordering of
             * the array is not significant because lookups use accountId. */
            list->items[i] = list->items[list->count - 1];
            list->count--;
            return CUST_OK;
        }
    }
    return CUST_ERR_NOTFOUND;
}

Customer *customerFindById(CustomerList *list, int accountId)
{
    int i;

    if (list == NULL) {
        return NULL;
    }
    for (i = 0; i < list->count; i++) {
        if (list->items[i].accountId == accountId) {
            return &list->items[i];
        }
    }
    return NULL;
}

const Customer *customerFindByIdConst(const CustomerList *list, int accountId)
{
    int i;

    if (list == NULL) {
        return NULL;
    }
    for (i = 0; i < list->count; i++) {
        if (list->items[i].accountId == accountId) {
            return &list->items[i];
        }
    }
    return NULL;
}

double customerGetBalance(const CustomerList *list, int accountId)
{
    const Customer *c = customerFindByIdConst(list, accountId);

    if (c == NULL) {
        return -1.0;                /* impossible as a real balance */
    }
    return c->balance;
}

int customerDeposit(CustomerList *list, int accountId, double amount)
{
    Customer *c;
    long balance;

    if (list == NULL || !validMoney(amount, 0.01, CUST_MAX_BALANCE))
        return CUST_ERR_INVALID;
    c = customerFindById(list, accountId);
    if (c == NULL)
        return CUST_ERR_NOTFOUND;
    balance = moneyCents(c->balance) + moneyCents(amount);
    if (balance > moneyCents(CUST_MAX_BALANCE))
        return CUST_ERR_INVALID;
    c->balance = balance / 100.0;
    return CUST_OK;
}

int customerCharge(CustomerList *list, int accountId, double amount)
{
    Customer *c;
    long balance, charge;

    if (list == NULL || !validMoney(amount, 0.0, CUST_MAX_BALANCE))
        return CUST_ERR_INVALID;
    c = customerFindById(list, accountId);
    if (c == NULL)
        return CUST_ERR_NOTFOUND;
    balance = moneyCents(c->balance);
    charge = moneyCents(amount);
    if (balance < charge)
        return CUST_ERR_INSUFFICIENT;
    c->balance = (balance - charge) / 100.0;
    return CUST_OK;
}

int customerAddOrder(CustomerList *list, int accountId)
{
    Customer *c;

    if (list == NULL) {
        return CUST_ERR_INVALID;
    }
    c = customerFindById(list, accountId);
    if (c == NULL) {
        return CUST_ERR_NOTFOUND;
    }
    if (c->orderCount == INT_MAX)
        return CUST_ERR_INVALID;
    c->orderCount++;
    return CUST_OK;
}

/* ---------------------------------------------------------------------------
 * Display helpers
 * ------------------------------------------------------------------------ */

void customerPrint(const Customer *c)
{
    if (c == NULL) {
        printf("  (no such account)\n");
        return;
    }
    printf("  %-8d %-20s %-16s %10.2f %8d\n",
           c->accountId, c->name, c->telephone, c->balance, c->orderCount);
}

void customerPrintAll(const CustomerList *list)
{
    int i;

    if (list == NULL) {
        return;
    }
    printf("\n  %-8s %-20s %-16s %10s %8s\n",
           "Account", "Name", "Telephone", "Balance", "Orders");
    printf("  -----------------------------------------------------------------\n");
    if (list->count == 0) {
        printf("  No customer accounts recorded.\n");
        return;
    }
    for (i = 0; i < list->count; i++) {
        customerPrint(&list->items[i]);
    }
    printf("  -----------------------------------------------------------------\n");
    printf("  Total accounts: %d\n", list->count);
}

int customerCount(const CustomerList *list)
{
    if (list == NULL) {
        return 0;
    }
    return list->count;
}

/* The first line stores the next ID. Older files contain only account rows. */
int customerLoad(CustomerList *list, const char *filename)
{
    FILE *file;
    CustomerList loaded;
    Customer item;
    char line[256];
    char *fields[5];
    int result, status, savedNextId;

    if (list == NULL || filename == NULL)
        return CUST_ERR_INVALID;
    file = openDataFile(filename);
    if (file == NULL)
        return errno == ENOENT ? CUST_OK : CUST_ERR_IO;

    customerListInit(&loaded);
    status = CUST_OK;
    savedNextId = 0;
    result = readTextLine(file, line, sizeof line);
    if (result == 1 && strncmp(line, "NEXT_ID|", 8) == 0) {
        if (!parseInt(line + 8, CUST_START_ID, INT_MAX, &savedNextId))
            status = CUST_ERR_IO;
        result = readTextLine(file, line, sizeof line);
    }

    while (status == CUST_OK && result == 1) {
        if (!splitFields(line, fields, 5) ||
            !parseInt(fields[0], CUST_START_ID, INT_MAX - 1, &item.accountId) ||
            !isValidName(fields[1]) || !isValidPhone(fields[2]) ||
            !parseMoney(fields[3], 0.0, CUST_MAX_BALANCE, &item.balance) ||
            !parseInt(fields[4], 0, INT_MAX, &item.orderCount) ||
            customerFindByIdConst(&loaded, item.accountId) != NULL) {
            status = CUST_ERR_IO;
            break;
        }

        status = ensureCapacity(&loaded);
        if (status != CUST_OK)
            break;
        copyField(item.name, CUST_NAME_LEN, fields[1]);
        copyField(item.telephone, CUST_PHONE_LEN, fields[2]);
        loaded.items[loaded.count++] = item;
        if (item.accountId >= loaded.nextId)
            loaded.nextId = item.accountId + 1;
        result = readTextLine(file, line, sizeof line);
    }
    if (status == CUST_OK && (result != -1 || ferror(file) ||
        (savedNextId != 0 && savedNextId < loaded.nextId)))
        status = CUST_ERR_IO;
    if (fclose(file) != 0)
        status = CUST_ERR_IO;
    if (status != CUST_OK) {
        customerListFree(&loaded);
        return status;
    }

    if (savedNextId != 0)
        loaded.nextId = savedNextId;
    customerListFree(list);
    *list = loaded;
    return CUST_OK;
}

int customerSave(const CustomerList *list, const char *filename)
{
    FILE *file;
    const Customer *item;
    char pending[DATA_PATH_LENGTH];
    int i;

    if (list == NULL || filename == NULL)
        return CUST_ERR_INVALID;
    file = beginDataSave(filename, pending);
    if (file == NULL)
        return CUST_ERR_IO;
    fprintf(file, "NEXT_ID|%d\n", list->nextId);
    for (i = 0; i < list->count; i++) {
        item = &list->items[i];
        fprintf(file, "%d|%s|%s|%.2f|%d\n", item->accountId, item->name,
                item->telephone, item->balance, item->orderCount);
    }
    return finishDataSave(file, filename, pending) ? CUST_OK : CUST_ERR_IO;
}

static void showPhoneFormat(void)
{
    printf("Phone number: at least %d digits and no more than %d characters total.\n",
           MIN_PHONE_DIGITS, CUST_PHONE_LEN - 1);
    puts("Digits, spaces, + and - are allowed. For a mainland China mobile,");
    puts("enter all 11 digits, for example 13800138000.");
}

int customerRegisterPrompt(CustomerList *list)
{
    char name[CUST_NAME_LEN], phone[CUST_PHONE_LEN];
    double amount;
    int id;

    puts("\nRegister a customer account");
    puts("The system will give you an account number when registration is complete.");
    if (promptLine("Name: ", name, sizeof name) != 1)
        return CUST_ERR_INVALID;
    if (!isValidName(name)) {
        puts("Enter a name. Do not include the | character.");
        return CUST_ERR_INVALID;
    }

    showPhoneFormat();
    if (promptLine("Phone number: ", phone, sizeof phone) != 1)
        return CUST_ERR_INVALID;
    if (!isValidPhone(phone)) {
        puts("Invalid phone number. Follow the format above and register again.");
        return CUST_ERR_INVALID;
    }

    puts("An initial deposit is optional. Enter 0 to start with an empty balance.");
    if (promptMoney("Initial deposit: $", 0.0,
                    CUST_MAX_BALANCE, &amount) != 1)
        return CUST_ERR_INVALID;

    id = customerRegister(list, name, phone, amount);
    if (id <= 0)
        puts("Registration failed. Check the name, phone and account limit.");
    return id;
}

int customerTopUpPrompt(CustomerList *list, int accountId)
{
    double amount;
    int status;

    printf("Available balance: $%.2f\n", customerGetBalance(list, accountId));
    puts("Enter the amount to add to your account, for example 20.00.");
    if (promptMoney("Amount to deposit: $", 0.01, CUST_MAX_BALANCE, &amount) != 1)
        return 0;
    status = customerDeposit(list, accountId, amount);
    if (status != CUST_OK) {
        puts("Deposit failed. Check the account and maximum balance.");
        return 0;
    }
    printf("New balance: $%.2f\n", customerGetBalance(list, accountId));
    return 1;
}

int customerManagerMenu(CustomerList *list)
{
    char name[CUST_NAME_LEN], phone[CUST_PHONE_LEN];
    int choice, id, status, confirm;
    const Customer *customer;

    puts("\n========== CUSTOMER ACCOUNTS ==========");
    puts("1. Add a new account");
    puts("2. Change a customer's name or phone number");
    puts("3. Remove a customer account");
    puts("4. List accounts and their account numbers\n0. Return");
    if (promptInt("Choice: ", 0, 4, &choice) != 1 || choice == 0)
        return 0;
    if (choice == 1) {
        id = customerRegisterPrompt(list);
        if (id > 0)
            printf("New account number: %d\n", id);
        return id > 0;
    }
    if (choice == 4) {
        customerPrintAll(list);
        return 0;
    }
    puts("Use the account number shown under List accounts.");
    if (promptInt("Account number (0 to return): ", 0, INT_MAX - 1, &id) != 1 ||
        id == 0)
        return 0;
    customer = customerFindByIdConst(list, id);
    if (customer == NULL) {
        puts("Account was not found. Choose List accounts to check the number.");
        return 0;
    }
    printf("Selected account: %d (%s)\n", customer->accountId, customer->name);

    if (choice == 2) {
        if (promptLine("New name (Enter to keep): ", name, sizeof name) != 1)
            return 0;
        showPhoneFormat();
        if (promptLine("New phone (Enter to keep): ", phone, sizeof phone) != 1)
            return 0;
        if (name[0] == '\0' && phone[0] == '\0') {
            puts("No changes entered.");
            return 0;
        }
        if (phone[0] != '\0' && !isValidPhone(phone)) {
            puts("Invalid phone number. Follow the format above and try again.");
            return 0;
        }
        status = customerEdit(list, id, name[0] ? name : NULL,
                              phone[0] ? phone : NULL);
    } else {
        puts("Removing this account will prevent the customer from signing in.");
        if (promptInt("Remove account? 1 = yes, 0 = cancel: ", 0, 1, &confirm) != 1 ||
            confirm == 0)
            return 0;
        status = customerDelete(list, id);
    }

    if (status != CUST_OK)
        puts("No change was made. Check the input.");
    else
        puts(choice == 2 ? "Account updated." : "Account removed.");
    return status == CUST_OK;
}
