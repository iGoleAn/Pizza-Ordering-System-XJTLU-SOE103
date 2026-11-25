Task B - Pizza Menu and Pricing (Visual Studio / MSVC C)

Files:
  menu.h       Shared public API for the team
  menu.c       Menu logic, price changes, file persistence, console helpers
  test_menu.c  Standalone test program only; remove from final team build

Visual Studio:
  Create a C++ Empty Project, then Add > Existing Item all three files.
  File extensions MUST remain .c (MSVC compiles them as C).
  Run with Ctrl+F5.

Integration:
  Include "menu.h" in main.c and order.c.
  Create one PizzaMenu variable in main; call menu_init(&menu).
  Call menu_load(&menu, "pizza_prices.txt") to restore previous manager edits.
  menu_calculate_price(&menu, crustIndex, sizeIndex, toppingIndex)
  returns a unit price, with ZERO-based indices, or -1 for invalid inputs.
  Multiply by order quantity in order.c; account deduction belongs to order.c.
  Manager permission checks belong to auth.c/main.c BEFORE calling
  menu_manager_interface or menu_set_price.
  Call menu_save after successful manager changes.
  Remove test_menu.c from the final build to avoid two main functions.

Example: crust=0 (Thin), size=1 (12 inch), topping=0 (Supreme)
         => 0.00 + 10.00 + 4.00 = $14.00

Prices and currency are illustrative, not specified by the assignment.
