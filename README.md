## Introduction

Table is a C++ project that provides functionality for managing and evaluating a table of cells. The cells can contain plain text or formulas that reference other cells to perform arithmetic calculations. The project uses a simple expression parser and evaluator to process the formulas and display results in a tabular format.

## Dependencies

- `iostream`: Input/output stream library.
- `string`: String library.
- `string_view`: String view library.
- `variant`: Variant library for handling different types.
- `FormulaAST.h`: Header file containing the formula abstract syntax tree.
- `cell.h`: Header file containing the CellInterface class.
- `common.h`: Header file containing common utility functions.
- `formula.h`: Header file containing formula-related functions.
- `test_runner_p.h`: Header file for the test runner.

## Usage

To use the Table project, you should include the necessary header files in your C++ application:

```cpp
#include <iostream>
#include <string>
#include <string_view>
#include <variant>
#include "FormulaAST.h"
#include "cell.h"
#include "common.h"
#include "formula.h"
#include "test_runner_p.h"
```

The project provides the following functionalities:

1. Managing Cells:
   - `CellInterface`: Represents a single cell in the table. It can contain plain text or formulas. Supports setting, getting, and clearing cell values.
   - `CreateSheet()`: Creates a new sheet containing cells.

2. Formula Evaluation:
   - `ParseFormula()`: Parses a formula expression and returns a pointer to the parsed formula.
   - `Formula::Evaluate()`: Evaluates a formula and returns the result.

3. Expression Formatting:
   - `ParseFormula()`: Parses a formula expression and returns a pointer to the parsed formula.
   - `Formula::GetExpression()`: Formats a parsed formula expression for display.

4. Error Handling:
   - The project supports various error categories, such as `FormulaError::Category::Value` and `FormulaError::Category::Div0`, which indicate errors during formula evaluation.

## Examples

Here are some examples of how to use the Table project:

### Managing Cells

```cpp
auto sheet = CreateSheet();
sheet->SetCell("A1"_pos, "Hello");
sheet->SetCell("A2"_pos, "=1+2");
sheet->ClearCell("A1"_pos);
```

### Formula Evaluation

```cpp
auto sheet = CreateSheet();
sheet->SetCell("A1"_pos, "2");
sheet->SetCell("A2"_pos, "3");
auto result = std::get<double>(ParseFormula("A1+A2")->Evaluate(*sheet));
std::cout << result; // Output: 5
```

### Expression Formatting

```cpp
std::string expr = "2 + 2*2";
auto formattedExpr = ParseFormula(expr)->GetExpression();
std::cout << formattedExpr; // Output: 2*2+2
```

### Error Handling

```cpp
auto sheet = CreateSheet();
sheet->SetCell("A1"_pos, "=1/0");
auto result = sheet->GetCell("A1"_pos)->GetValue();
if (std::holds_alternative<FormulaError>(result)) {
    std::cout << "Error: " << std::get<FormulaError>(result).ToString();
}
```

## Testing

The project includes a test suite to ensure the correctness of its functionalities. To run the tests, use the provided `test_runner_p.h` header file and execute the test functions.

```cpp
#include "test_runner_p.h"

int main() {
    TestRunner tr;
    RUN_TEST(tr, TestPositionAndStringConversion);
    RUN_TEST(tr, TestPositionToStringInvalid);
    // Add more test functions here if needed
    return 0;
}
```

## Dear practicum.yandex.ru team,

Thank you for the excellent C++ development course. It was engaging, informative, and provided me with valuable skills. Grateful for the opportunity to learn with you.
