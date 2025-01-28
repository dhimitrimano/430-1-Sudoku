# Sudoku

A Sudoku validator program using multiple POSIX threads to speed up the process.

### Description

The program validates sudoku puzzles by creating threads that check the following criteria:

- A thread to check that each column contains the digits 1 through 9
- A thread to check that each row contains the digits 1 through 9
- Nine threads to check that each of the 3 x 3 subgrids contains the digits 1 through 9

---

Credits:<br>
Starter code provided by Yusuf Pisan<br>
Sudoku validator implementation by Dhimitri Mano
