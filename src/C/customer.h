/*
 * ============================================================================
 * File        : customer.h
 * Module      : C - Customer Account System
 * Course      : SOE103 C Programming and Software Engineering
 * Project     : Pizza Ordering System (Group Project)
 * Description : Public interface of the customer account module.
 *               It provides registration, editing, deletion, balance enquiry,
 *               top-up (deposit) and unique account number management.
 *               This header is the ONLY file other modules (main.c, order.c,
 *               storage.c) need to include in order to use this module.
 * ============================================================================
 */

#ifndef CUSTOMER_H
#define CUSTOMER_H

/* ---------------------------------------------------------------------------
 * Configuration constants
 * ------------------------------------------------------------------------ */
#define CUST_NAME_LEN       50      /* max characters in a customer name      */
#define CUST_PHONE_LEN      20      /* max characters in a telephone number   */
#define CUST_START_ID       1001    /* first account number issued            */
#define CUST_MAX_BALANCE    1000000.0  /* upper bound to avoid overflow       */
#define CUST_MAX_RECORDS    500     /* safety cap on the number of accounts   */
#define CUST_DATA_FILE      "customers.dat"  /* default persistence file      */

/* ---------------------------------------------------------------------------
 * Status codes returned by the module.
 * A negative value always means failure; CUST_OK (0) means success.
 * Note: customerRegister() returns a positive account number on success.
 * ------------------------------------------------------------------------ */
typedef enum {
    CUST_OK               =  0,  /* operation completed successfully          */
    CUST_ERR_NOTFOUND     = -1,  /* no account with the requested number      */
    CUST_ERR_INVALID      = -2,  /* invalid argument or invalid user input    */
    CUST_ERR_INSUFFICIENT = -3,  /* balance too low to complete a payment     */
    CUST_ERR_NOMEM        = -4,  /* dynamic memory allocation failed          */
    CUST_ERR_IO           = -5,  /* file read / write error                   */
    CUST_ERR_DUPLICATE    = -6   /* reserved for future use              */
} CustStatus;

/* ---------------------------------------------------------------------------
 * Customer record.
 * The order history is deliberately NOT stored here: order.c owns the order
 * records.  Each order stores the accountId, so order.c can build a customer's
 * history by scanning its own list.  orderCount is only a cached counter that
 * order.c keeps up to date through customerAddOrder().
 * ------------------------------------------------------------------------ */
typedef struct {
    int    accountId;                       /* unique, auto-generated number  */
    char   name[CUST_NAME_LEN];             /* customer full name             */
    char   telephone[CUST_PHONE_LEN];       /* contact telephone number       */
    double balance;                         /* available balance, always >= 0 */
    int    orderCount;                      /* number of orders placed        */
} Customer;

/* ---------------------------------------------------------------------------
 * The whole customer database.  A dynamic array is used so that the module can
 * be compiled and tested on its own, without depending on storage.c.
 * ------------------------------------------------------------------------ */
typedef struct {
    Customer *items;      /* dynamically allocated array of Customer records */
    int       count;      /* number of records currently in use              */
    int       capacity;   /* number of allocated slots                       */
    int       nextId;     /* account number for the next registration        */
} CustomerList;

/* ---------------------------------------------------------------------------
 * Life-cycle management
 * ------------------------------------------------------------------------ */
void customerListInit(CustomerList *list);
/* Initialise an EMPTY list.  Must be called before any other function. */

void customerListFree(CustomerList *list);
/* Release the memory owned by the list.  Safe to call more than once. */

/* ---------------------------------------------------------------------------
 * Core operations required by the specification
 * ------------------------------------------------------------------------ */
int customerRegister(CustomerList *list, const char *name,
                     const char *telephone, double initialDeposit);
/* Create a new account with a unique number.
 * Returns : the new (positive) account number on success,
 *           a negative CustStatus code on failure.
 * Validation: name not empty, telephone contains at least 6 digits,
 *             initialDeposit in [0, CUST_MAX_BALANCE].                     */

int customerEdit(CustomerList *list, int accountId,
                 const char *newName, const char *newTelephone);
/* Change the name and/or telephone of an existing account.
 * Passing NULL for a field leaves that field unchanged.
 * Returns CUST_OK or a negative CustStatus code.                          */

int customerDelete(CustomerList *list, int accountId);
/* Permanently remove an account.  Account numbers are never reused.
 * Returns CUST_OK or a negative CustStatus code.                          */

Customer       *customerFindById(CustomerList *list, int accountId);
const Customer *customerFindByIdConst(const CustomerList *list, int accountId);
/* Linear search by unique account number.  NULL if not found.             */

double customerGetBalance(const CustomerList *list, int accountId);
/* Return the balance, or -1.0 when the account does not exist.
 * (Balances are never negative, so -1.0 is an unambiguous error value.)   */

int customerDeposit(CustomerList *list, int accountId, double amount);
/* Top up an account.  amount must be > 0 and must not push the balance
 * above CUST_MAX_BALANCE.  Returns CUST_OK or a negative status code.     */

int customerCharge(CustomerList *list, int accountId, double amount);
/* Deduct money when an order is placed.  This is the interface called by
 * order.c (module D).  Fails with CUST_ERR_INSUFFICIENT if the balance is
 * too low. A zero-price order leaves the balance unchanged.
 * Returns CUST_OK or a negative status code.                    */

int customerAddOrder(CustomerList *list, int accountId);
/* Increment the cached order counter after a successful order.  Called by
 * order.c (module D).                                                     */

/* ---------------------------------------------------------------------------
 * Display helpers
 * ------------------------------------------------------------------------ */
void customerPrint(const Customer *c);
/* Print one customer record in a single aligned line. */

void customerPrintAll(const CustomerList *list);
/* Print every customer in a table, including a header and a total. */

int customerCount(const CustomerList *list);
/* Return the number of accounts currently stored. */

/* ---------------------------------------------------------------------------
 * Persistence (may be called by storage.c / main.c)
 * ------------------------------------------------------------------------ */
int customerLoad(CustomerList *list, const char *filename);
/* Load accounts from a '|'-separated text file.  A missing file is treated
 * as an empty database (CUST_OK). The next ID is restored from the header.
 * Older files without a header use the largest existing ID plus one. */

int customerSave(const CustomerList *list, const char *filename);
/* The first line is NEXT_ID|number, then account rows are stored as:
 *   accountId|name|telephone|balance|orderCount
 * Returns CUST_OK or CUST_ERR_IO.                                         */

/* ---------------------------------------------------------------------------
 * Interactive front-end
 * ------------------------------------------------------------------------ */
int customerRegisterPrompt(CustomerList *list);
/* Top-up only touches the account selected by the caller after login. */
int customerTopUpPrompt(CustomerList *list, int accountId);
/* Manager actions return 1 if the account list changed. */
int customerManagerMenu(CustomerList *list);

#endif /* CUSTOMER_H */
