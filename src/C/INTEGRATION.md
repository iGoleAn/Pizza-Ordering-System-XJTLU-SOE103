# Module C integration

A/main.c owns one CustomerList. It calls customerListInit and customerLoad on
startup, then customerListFree on exit. Account changes made in A/B/C/E mode
are saved by A. Account data uses customers.dat by default.

A checks the customer account number and telephone on login. The customer's
balance and top-up actions use the authenticated account ID passed by A.
The manager uses customerManagerMenu for add, edit, remove and list.

## For Module D

- Get the declarations and order file format from A/order_port.h. The full
  build commands are in the src README.
- Use the same CustomerList pointer and logged-in ID passed by A.
- Call customerCharge after validating and confirming the order. It returns
  CUST_OK (0), CUST_ERR_INSUFFICIENT, CUST_ERR_NOTFOUND or CUST_ERR_INVALID.
  A zero-price order is valid. Balances and charges use cents internally.
- Call customerAddOrder for a successful order and save the account list with
  customerSave. Check both return codes. The order file and account changes
  must agree after a storage failure.
- D stores full order history. C stores only the count. When a manager deletes
  an account, its old orders can still be included in sales figures.

## Account file

The first line is NEXT_ID|number. Each following line uses:

~~~text
accountId|name|telephone|balance|orderCount
1001|Alice Chen|13800138000|50.00|2
~~~

A legacy file without the first line is read using the largest existing ID
plus one. Previously deleted IDs cannot be recovered from that older format.
New saves retain the next ID, including after deletion of the latest account.
A malformed file is reported without replacing the current in-memory list.

## Separate test build

From src/C with GCC:

~~~sh
gcc -std=c90 -pedantic -Wall -Wextra ../A/input.c ../A/datafile.c customer.c test_customer.c -o test_customer
~~~

Run the driver separately from the application since each has a main.
