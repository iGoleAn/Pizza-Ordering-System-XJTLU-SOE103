# Module C — Customer Account System

**Course:** SOE103 C Programming and Software Engineering
**Group Project:** Pizza Ordering System
**Contributor:** Module C (Customer Account System)
**Source files:** `customer.h`, `customer.c`
**Test driver:** `test_customer.c`

---

## 1. Problem Statement

The customer account system is one of five modules of the Pizza Ordering System.
It is responsible for all operations concerning customer accounts. According to
the design brief, the ordering system must be able to *"register and edit customer
accounts, identifiable by a unique account number, which can be pre-charged with
money. Customer information should include name, telephone, order history and
available balance."* It must also let a customer *"deposit money on the account"*
and *"review order history"*, and it must let the shop manager *"add/remove/edit
customer information"*.

After analysing the brief and agreeing the interfaces with the rest of the group,
Module C is required to provide the following functions:

1. **Register** a new customer account, storing the customer's name and telephone
   number, together with an optional initial deposit.
2. Assign a **unique account number** to every account. Numbers are never reused,
   not even after an account is deleted or the program is restarted.
3. **Edit** the name and/or telephone number of an existing account.
4. **Delete** an account.
5. **Enquire the balance** (available money) of an account.
6. **Top up (deposit)** money into an account.
7. Provide a **charge function** that Module D (order.c) calls to deduct money
   when an order is placed, and an **order-counter function** to maintain the
   order history count.
8. **Load and save** all account records to a data file so information survives
   between program runs.
9. Provide a **menu-driven interface** that behaves differently for a manager
   (full access) and a customer (view and top-up only).

The module is deliberately self-contained so it can be compiled and tested
independently before being integrated into the main program by Module A.

---

## 2. Analysis

### 2.1 Data to be stored

Each customer record contains the following fields:

| Field | Type | Purpose |
|---|---|---|
| `accountId` | `int` | Unique identifier, generated automatically |
| `name` | `char[50]` | Customer's full name |
| `telephone` | `char[20]` | Contact telephone number |
| `balance` | `double` | Available money (always ≥ 0) |
| `orderCount` | `int` | Number of orders placed |

The full **order history** is *not* stored inside this module. Order records are
owned by Module D (`order.c`); each order simply stores the `accountId` of the
customer who placed it. Module D can therefore reconstruct a customer's complete
history by scanning its own order list. This separation avoids duplicated data
and keeps the two modules only loosely coupled. Module C keeps an `orderCount`
as a cached counter, updated by Module D through `customerAddOrder()`.

### 2.2 Data structure

A **dynamic array** of `Customer` structures (`CustomerList`) is used, rather
than a fixed-size array or a linked list. The reasons are:

- The number of customers is not known in advance, so the array grows with
  `realloc()` when full (doubling its capacity each time for efficiency).
- A contiguous array makes sequential display and file I/O straightforward.
- Random access by index is simple, and a linear search by `accountId` is fast
  enough for the expected number of customers (a few hundred at most).

The structure also holds `nextId`, the account number to be assigned to the next
customer. This is the key to guaranteeing uniqueness.

### 2.3 Unique account number strategy

Numbers start at `CUST_START_ID` (1001) and increase by one for each new
registration. Two rules guarantee that a number is never duplicated:

1. `nextId` is incremented every time an account is created and is **never
   decremented**, so deleting an account cannot free its number for reuse.
2. Saving writes the next ID to the first line of the file. Loading restores
   that number, including after the highest account was deleted. A legacy
   file without this line uses the largest remaining account ID plus one.

### 2.4 Input validation and robustness

Invalid data must never corrupt the account database. The following rules are
applied:

- A name must contain at least one non-blank character and must not contain the
  field separator `'|'` or a newline (which would corrupt the data file).
- A telephone number may contain digits, spaces, `+` and `-`, but must contain
  at least six digits.
- Initial deposits may be zero and top-ups must be positive. Both are
  validated to cents, and the balance may not exceed `CUST_MAX_BALANCE`.
  Balance updates use whole cents so the balance can be spent exactly.
- A charge fails with `CUST_ERR_INSUFFICIENT` if the balance is too low, so a
  customer can never end up with a negative balance.
- Account operations check the supplied list and return a documented status
  when a requested change fails.
- In the interactive menu, input is read with `fgets()` and parsed with
  `strtol()`/`strtod()` instead of raw `scanf()`. This prevents an invalid
  character from being left in the input buffer and causing an infinite loop
  on the next read — a common robustness defect.

### 2.5 Interface with other modules

| Function | Called by | Purpose |
|---|---|---|
| `customerListInit()` / `customerListFree()` | main.c (A) | start-up / shut-down |
| `customerLoad()` / `customerSave()` | main.c (A) | load on startup and save account changes |
| `customerRegisterPrompt()` / `customerTopUpPrompt()` / `customerManagerMenu()` | main.c (A) | register, top up the signed-in account, or manage accounts |
| `customerCharge()` | order.c (D) | deduct money when an order is placed |
| `customerAddOrder()` | order.c (D) | update the order count after an order |
| `customerFindByIdConst()` | main.c (A) | look up the account and check the registered phone |

---

## 3. Algorithm Design

### 3.1 Functional structure

The module is divided into four groups of functions:

```
                      customer.c (Module C)
                              |
        +---------------------+---------------------+
        |                     |                     |
   Core operations      Persistence          Interactive UI
   ---------------      -----------          -------------
   register              load                 accountMenu
   edit                  save                   /  |
   delete                                       /   |
   findById                                 customerPrint
   getBalance                                customerPrintAll
   deposit
   charge
   addOrder
```

### 3.2 Flow diagram — customer registration

```
                  Start
                    |
          read name, phone, deposit
                    |
        name valid & phone valid &
        deposit in [0, MAX] ?
             /            \
           No              Yes
            |               |
     return CUST_ERR_INVALID   |
                            has room in array?
                          /            \
                        No              Yes
                         |               |
                  grow with realloc()   |
                  (or return NOMEM)     |
                               \        /
                                \      /
                           id = nextId++
                    store fields, count++
                             |
                      return id (> 0)
                             |
                            End
```

### 3.3 Flow diagram — top-up (deposit)

```
                Start
                  |
        read accountId, amount
                  |
          amount > 0 ?
          /         \
        No           Yes
         |            |
    return INVALID   find account by id
                     /         \
                 Not found     Found
                    |            |
             return NOTFOUND  balance + amount <= MAX ?
                              /            \
                            No              Yes
                             |               |
                       return INVALID    balance += amount
                                              |
                                         return CUST_OK
                                              |
                                             End
```

### 3.4 Flow diagram — charge (called by Module D)

```
                Start
                  |
          find account by id
             /         \
       Not found       Found
          |              |
   return NOTFOUND   balance >= amount ?
                     /          \
                   No            Yes
                    |             |
          return INSUFFICIENT  balance -= amount
                                   |
                              return CUST_OK
                                   |
                                  End
```

### 3.5 Data passed between functions

`CustomerList *` is passed by pointer to every modifying function so changes are
visible to the caller. `const CustomerList *` is used for read-only functions
(`customerGetBalance`, `customerFindByIdConst`, `customerPrintAll`), which makes
the contract explicit and prevents accidental modification. Account lookup is a
linear search over `list->items[0..count-1]`, returning a pointer to the matching
record (or `NULL`). Returning a pointer, rather than a copy, allows Module D to
update the record directly through the provided functions.

---

## 4. Implementation

The module is implemented in `customer.h` (public interface, constants, data
types and status codes) and `customer.c` (all function bodies). Both files are
written in C89-compatible style with full comments so that any laboratory
compiler can build them.

Key implementation points:

- **Safe string copying** — `copyField()` copies into a fixed-size field and
  always writes a terminating `'\0'`, so over-long input cannot overflow the
  `name` or `telephone` arrays.
- **Geometric growth** — `ensureCapacity()` doubles the array when full, giving
  amortised O(1) insertion, and respects the `CUST_MAX_RECORDS` safety limit. A
  failed `realloc()` leaves the original array intact.
- **Deletion** — the removed record is replaced by the last record in the array
  and `count` is decremented. Ordering is irrelevant because lookups are by
  `accountId`.
- **Persistence format** — the header is `NEXT_ID|number`. It is followed
  by one `accountId|name|telephone|balance|orderCount` row per customer.
  Loading checks rows before replacing the current list.
- **Error reporting** — account changes return `CUST_OK` (0) or a negative
  status. Registration returns the new account number on success.

---

## 5. Testing

### 5.1 Test approach

Module C was tested independently before integration. A standalone test driver
(`test_customer.c`) calls the account functions and checks the returned status
and the resulting data. Both valid and invalid inputs are exercised, following
the SDP guidance that invalid inputs are especially important. The test file
compiles with the account and shared input code.

```
gcc -Wall -Wextra -std=c89 -pedantic -o test_customer ../A/input.c ../A/datafile.c customer.c test_customer.c
```

### 5.2 Test accounts

The accounts used for testing are listed below (no passwords are used by this
module; login authentication is handled by Module A):

| Account No. | Name | Telephone | Initial balance |
|---|---|---|---|
| 1001 | Alice Chen | 13800138000 | 50.00 |
| 1002 | Bob Wang | 13900139000 | 0.00 |
| 1003 | Carol Liu | 13600136000 | 10.00 |

### 5.3 Test cases

The separate C driver was compiled and run with MSVC. All 44 checks passed.

| Test IDs | Coverage |
|---|---|
| T01–T08 | Empty list, sequential IDs and invalid registration data |
| T09–T19 | Balance, deposits, normal charges and insufficient funds |
| T20–T29 | Order count, edit, remove and monotonic IDs. T24 rejects a name containing the file delimiter. |
| T30–T38 | Save and reload, including deletion of the last issued ID before restart |
| T39–T42 | Non-finite or sub-cent amounts, exact-cent deduction and zero-price orders |
| T43–T44 | Legacy data loads and a duplicate-ID file is rejected without replacing the current list |

### 5.4 Additional tests recommended for integration

Once the modules are combined, the following should also be checked:

- Placing an order through Module D reduces the customer balance by the order
  total and increments the order count.
- Reviewing order history displays exactly the orders belonging to the logged-in
  account.
- Data saved on exit is correctly restored at the next start-up.
- A manager can edit/delete any account, while a customer cannot access those
  menu options.

### 5.5 Known limitations

- Account numbers are sequential and predictable; this is acceptable for an
  in-store demonstration system but would not be acceptable for a public system.
- The data file is plain text; no encryption is applied to telephone numbers.
  This is acceptable for the coursework scenario.
- Deleting a customer does not delete that customer's historical orders held by
  Module D; the group should decide whether history must be retained for sales
  statistics (Module E) after an account is removed.
