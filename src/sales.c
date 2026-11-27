#include "sales.h"
#include "../A/input.h"
#include "../D/order_internal.h"
#include "../A/datafile.h"
#include <errno.h>
#include <limits.h>
#include <string.h>

void salesDisplay(const char *filename)
{
    FILE *file;
    char line[256];
    long orderCount = 0, pizzaCount = 0;
    long pizzasBySize[MENU_MAX_SIZE + 1] = {0};
    double totalSales = 0.0;
    OrderRecord order;
    double salesBySize[MENU_MAX_SIZE + 1] = {0.0};
    int size, result, ok, lastId = 0;

    puts("\n========== SALES STATISTICS ==========");
    file = openDataFile(filename);
    if (file == NULL) {
        if (errno == ENOENT)
            puts("No completed orders yet.");
        else
            puts("The order file could not be opened.");
        return;
    }

    /* Scan each completed order once. Use the price saved at checkout. */
    result = readTextLine(file, line, sizeof line);
    while (result == 1 && orderReadRow(line, &order) && order.orderId > lastId) {
        lastId = order.orderId;
        size = order.sizeInches;
        if (pizzaCount > LONG_MAX - order.quantity)
            break;
        orderCount++;
        pizzaCount += order.quantity;
        totalSales += order.total;
        pizzasBySize[size] += order.quantity;
        salesBySize[size] += order.total;
        result = readTextLine(file, line, sizeof line);
    }
    ok = result == -1 && !ferror(file);
    if (fclose(file) != 0)
        ok = 0;
    if (!ok) {
        printf("Cannot calculate sales. Check order row %ld.\n", orderCount + 1);
        return;
    }
    if (orderCount == 0) {
        puts("No completed orders yet.");
        return;
    }

    printf("Completed orders: %ld\nPizzas sold: %ld\nSales revenue: $%.2f\n",
           orderCount, pizzaCount, totalSales);
    printf("\n%-10s %10s %12s\n", "Size", "Pizzas", "Revenue");
    for (size = 1; size <= MENU_MAX_SIZE; size++) {
        if (pizzasBySize[size] > 0)
            printf("%2d inch    %10ld $%11.2f\n", size,
                   pizzasBySize[size], salesBySize[size]);
    }
}
