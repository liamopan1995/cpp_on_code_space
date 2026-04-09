# basic-calculator Specification

## Purpose
TBD - created by archiving change create-basic-calculator. Update Purpose after archive.
## Requirements
### Requirement: Calculator shall perform addition
The calculator SHALL provide an add function that takes two numbers and returns their sum.

#### Scenario: Adding positive numbers
- **WHEN** add(2.0, 3.0) is called
- **THEN** it returns 5.0

#### Scenario: Adding negative numbers
- **WHEN** add(-2.0, -3.0) is called
- **THEN** it returns -5.0

### Requirement: Calculator shall perform subtraction
The calculator SHALL provide a subtract function that takes two numbers and returns their difference.

#### Scenario: Subtracting positive numbers
- **WHEN** subtract(5.0, 3.0) is called
- **THEN** it returns 2.0

#### Scenario: Subtracting resulting in negative
- **WHEN** subtract(3.0, 5.0) is called
- **THEN** it returns -2.0

### Requirement: Calculator shall perform multiplication
The calculator SHALL provide a multiply function that takes two numbers and returns their product.

#### Scenario: Multiplying positive numbers
- **WHEN** multiply(2.0, 3.0) is called
- **THEN** it returns 6.0

#### Scenario: Multiplying with zero
- **WHEN** multiply(5.0, 0.0) is called
- **THEN** it returns 0.0

### Requirement: Calculator shall perform division
The calculator SHALL provide a divide function that takes two numbers and returns their quotient.

#### Scenario: Dividing positive numbers
- **WHEN** divide(6.0, 2.0) is called
- **THEN** it returns 3.0

#### Scenario: Division by zero
- **WHEN** divide(5.0, 0.0) is called
- **THEN** it throws a division_by_zero exception

