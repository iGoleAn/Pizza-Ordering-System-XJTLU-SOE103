/*
 * ============================================================================
 * File        : test_customer.c
 * Module      : C - Customer Account System (independent test driver)
 * Description : Standalone test harness for customer.c.  It exercises every
 *               public function and checks both valid and invalid inputs.
 *               Compile together with customer.c:
 *                   gcc -Wall -Wextra -std=c89 -o test_customer ../A/input.c ../A/datafile.c customer.c test_customer.c
 * ============================================================================
 */

#include "customer.h"
#include <stdio.h>
#include <math.h>
#include <string.h>     /* strcmp                                            */

static int passed = 0;
static int failed = 0;

static void check(int condition, const char *message)
{
    if (condition) {
        passed++;
        printf("  [PASS] %s\n", message);
    } else {
        failed++;
        printf("  [FAIL] %s\n", message);
    }
}

int main(void)
{
    CustomerList list;
    int id1, id2, id3, id4;
    FILE *file;
    double balance;
    const Customer *c;

    printf("=== Module C: Customer Account System - unit tests ===\n");

    customerListInit(&list);
    check(customerCount(&list) == 0, "T01 new list is empty");

    /* --- registration and unique account numbers --- */
    id1 = customerRegister(&list, "Alice Chen", "13800138000", 50.0);
    check(id1 == CUST_START_ID, "T02 first account gets the starting number");
    id2 = customerRegister(&list, "Bob Wang", "13900139000", 0.0);
    check(id2 == id1 + 1, "T03 second account gets a different unique number");
    check(customerCount(&list) == 2, "T04 two accounts stored");

    /* --- validation on registration --- */
    check(customerRegister(&list, "", "13800138000", 10.0) < 0,
          "T05 empty name rejected");
    check(customerRegister(&list, "Carol", "123", 10.0) < 0,
          "T06 too-short telephone rejected");
    check(customerRegister(&list, "Carol", "13700137000", -5.0) < 0,
          "T07 negative deposit rejected");
    check(customerCount(&list) == 2, "T08 invalid registrations did not add records");

    /* --- balance enquiry --- */
    balance = customerGetBalance(&list, id1);
    check(balance == 50.0, "T09 balance enquiry returns the deposited amount");
    check(customerGetBalance(&list, 9999) < 0.0,
          "T10 balance enquiry on unknown account returns -1");

    /* --- deposit --- */
    check(customerDeposit(&list, id1, 25.5) == CUST_OK,
          "T11 valid top-up succeeds");
    check(customerGetBalance(&list, id1) == 75.5,
          "T12 balance updated after top-up");
    check(customerDeposit(&list, id1, -10.0) == CUST_ERR_INVALID,
          "T13 negative top-up rejected");
    check(customerDeposit(&list, id1, 0.0) == CUST_ERR_INVALID,
          "T14 zero top-up rejected");
    check(customerDeposit(&list, 9999, 10.0) == CUST_ERR_NOTFOUND,
          "T15 top-up on unknown account rejected");

    /* --- charge (interface used by order.c) --- */
    check(customerCharge(&list, id1, 20.0) == CUST_OK,
          "T16 charge succeeds when balance is sufficient");
    check(customerGetBalance(&list, id1) == 55.5,
          "T17 balance reduced after charge");
    check(customerCharge(&list, id2, 10.0) == CUST_ERR_INSUFFICIENT,
          "T18 charge rejected when balance is insufficient");
    check(customerCharge(&list, 9999, 10.0) == CUST_ERR_NOTFOUND,
          "T19 charge on unknown account rejected");

    /* --- order counter --- */
    check(customerAddOrder(&list, id1) == CUST_OK, "T20 order counter incremented");
    c = customerFindByIdConst(&list, id1);
    check(c != NULL && c->orderCount == 1, "T21 order counter value correct");

    /* --- edit --- */
    check(customerEdit(&list, id1, "Alice Zhang", NULL) == CUST_OK,
          "T22 name change succeeds");
    c = customerFindByIdConst(&list, id1);
    check(c != NULL && strcmp(c->name, "Alice Zhang") == 0,
          "T23 new name stored");
    check(customerEdit(&list, id1, "A|B", NULL) < 0,
          "T24 invalid new name rejected");
    check(customerEdit(&list, 9999, "Nobody", NULL) == CUST_ERR_NOTFOUND,
          "T25 edit on unknown account rejected");

    /* --- delete --- */
    check(customerDelete(&list, 9999) == CUST_ERR_NOTFOUND,
          "T26 delete on unknown account rejected");
    check(customerDelete(&list, id2) == CUST_OK, "T27 delete existing account succeeds");
    check(customerCount(&list) == 1, "T28 account count decreased after deletion");

    /* --- account numbers are never reused --- */
    id3 = customerRegister(&list, "Carol Liu", "13600136000", 10.0);
    check(id3 > id2, "T29 deleted account number is not reused");

    /* --- persistence --- */
    check(customerSave(&list, "test_customers.dat") == CUST_OK,
          "T30 save to file succeeds");
    customerListFree(&list);
    customerListInit(&list);
    check(customerLoad(&list, "test_customers.dat") == CUST_OK,
          "T31 load from file succeeds");
    check(customerCount(&list) == 2, "T32 two accounts restored");
    check(customerGetBalance(&list, id1) == 55.5,
          "T33 restored balance is correct");
    c = customerFindByIdConst(&list, id1);
    check(c != NULL && c->orderCount == 1, "T34 restored order count is correct");
    id4 = customerRegister(&list, "Dave Xu", "13500135000", 1.0);
    check(id4 > id3, "T35 account numbers stay unique after reload");
    check(customerDelete(&list, id4) == CUST_OK,
          "T36 delete the most recently issued account");
    check(customerSave(&list, "test_customers.dat") == CUST_OK &&
          customerLoad(&list, "test_customers.dat") == CUST_OK,
          "T37 reload replaces the existing list");
    check(customerCount(&list) == 2 &&
          customerRegister(&list, "Evan Wu", "13400134000", 0.30) == id4 + 1,
          "T38 the deleted account number is not reissued after restart");
    check(customerDeposit(&list, id1, HUGE_VAL) == CUST_ERR_INVALID &&
          customerGetBalance(&list, id1) == 55.50,
          "T39 a non-finite deposit leaves the balance unchanged");
    check(customerDeposit(&list, id1, 0.001) == CUST_ERR_INVALID,
          "T40 rejects fractions smaller than one cent");
    check(customerCharge(&list, id4 + 1, 0.10) == CUST_OK &&
          customerCharge(&list, id4 + 1, 0.20) == CUST_OK &&
          customerGetBalance(&list, id4 + 1) == 0.0,
          "T41 subtracts exact cents");
    check(customerCharge(&list, id4 + 1, 0.0) == CUST_OK,
          "T42 accepts a zero-price order");
    file = fopen("test_customers.dat", "w");
    if (file != NULL) {
        fputs("1001|Alice Chen|13800138000|50.00|2\n", file);
        fclose(file);
    }
    check(file != NULL && customerLoad(&list, "test_customers.dat") == CUST_OK &&
          customerCount(&list) == 1 && list.nextId == 1002,
          "T43 loads the old account format");
    file = fopen("test_customers.dat", "a");
    if (file != NULL) {
        fputs("1001|Duplicate|13800138000|30.00|1\n", file);
        fclose(file);
    }
    check(file != NULL && customerLoad(&list, "test_customers.dat") == CUST_ERR_IO &&
          customerCount(&list) == 1 && customerGetBalance(&list, id1) == 50.0,
          "T44 a damaged file does not replace the current list");
    remove("test_customers.dat");

    customerPrintAll(&list);
    customerListFree(&list);

    printf("\n=== Result: %d passed, %d failed ===\n", passed, failed);
    return (failed == 0) ? 0 : 1;
}
