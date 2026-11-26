# SOE103 Pizza Ordering System

Run the program from the src directory. Module A contains the main program and shared input functions. Module B manages the pizza menu, Module C manages customer accounts, and Module E reports sales statistics.

## Run Modules A, B, C, and E

Double-click pizza.exe in the src directory, or open PowerShell in that directory and run:

~~~powershell
.\pizza.exe
~~~

| Role | How to sign in |
| --- | --- |
| Customer | Select Register customer account and enter your name, phone number, and initial deposit. Sign in with the account number shown on screen and the phone number you entered. |
| Manager | Sign in with the username manager and the demo password Pizza103. |

Start the program from the same working directory each time. It reads or creates customers.dat and pizza_prices.txt there. Run one instance at a time. Temporary .new and .bak files may appear while data is being saved.

Customers can check their balance, deposit money, and preview pizza prices. Module E reads orders.txt in the format described below. If there are no orders, it displays "No completed orders yet."

## Integrate and Run Module D

1. Put the Module D implementation in the D directory. The application uses the main function in A/main.c. Compile any standalone tests for Module D separately.
2. Include "../A/order_port.h" in the Module D implementation and provide the two functions declared there. If D already uses different function names, add two small wrapper functions that call them. orderPlace receives the active menu, customer list, and signed-in account number from Module A. orderShowHistory displays that account's order history.
3. Use ORDER_DATA_FILE to access orders.txt in the current working directory. Write one row for each successful order, with the fields in the order shown below. The file has no header row. See order_port.h for the quantity limit and total price range.

Field order: orderId|accountId|crustName|sizeInches|toppingName|quantity|total

~~~text
1|1001|Thin and Crispy|12|Supreme|2|28.00
~~~

4. Use menu_choose_pizza and menu_calculate_price from Module B. Use customerCharge, customerAddOrder, and customerSave from Module C to update and save the account. Check the balance, ask the customer to confirm, then submit the order. Handle errors when saving either file. The sales file should contain only successful orders. Store the selected names, size, and total price at checkout so past sales remain accurate after a menu price changes or a customer account is removed.
5. From the same src directory, build and run with one of the commands below. These examples assume both Module D functions are in D/order.c. If D uses several implementation files, add the other required .c files to the command.

~~~bat
cl /nologo /W4 /TC /D_CRT_SECURE_NO_WARNINGS /DWITH_ORDERS /Fe:pizza.exe A\main.c A\input.c A\datafile.c B\menu.c C\customer.c D\order.c E\sales.c
pizza.exe
~~~

~~~powershell
gcc -std=c90 -pedantic -Wall -Wextra -DWITH_ORDERS A/main.c A/input.c A/datafile.c B/menu.c C/customer.c D/order.c E/sales.c -o pizza.exe
.\pizza.exe
~~~

If using an empty Visual Studio project, add the same .c files and compile them as C. After adding the Module D implementation, add WITH_ORDERS under C/C++ → Preprocessor Definitions. Set the debugging Working Directory to src.

After integration, customers can place an order with option 4 and view their order history with option 5. Try one order with sufficient balance and another with insufficient balance. Check the balance, order history, and manager sales totals, then restart the program and confirm that the account and order records are restored.

## Run the Module B and C Tests Separately

Run the following commands from the src directory in a Visual Studio Developer Command Prompt. The tests create temporary data files and overwrite any files with the same test filenames.

~~~bat
cl /nologo /W4 /TC /D_CRT_SECURE_NO_WARNINGS /Fe:test_menu.exe A\input.c A\datafile.c B\menu.c B\test_menu.c
test_menu.exe
cl /nologo /W4 /TC /D_CRT_SECURE_NO_WARNINGS /Fe:test_customer.exe A\input.c A\datafile.c C\customer.c C\test_customer.c
test_customer.exe
~~~
