# 5300-Antelope
Student DB Relation Manager project for CPSC5300 at Seattle U, Winter 2024

## Setup & Requirements
To setup this project, clone this repo into `~/cpsc5300` on `cs1`.

After cloning, this is what your directory should look like:
```
~/
└── cpsc5300/
    ├── data/
    └── 5300-Antelope/
```

## Building & Usage
Go to the project directory:

`$ cd ~/cpsc5300/5300-Antelope`

To run the SQL Parser from Milestone 1:

`$ make clean && make && ./sql5300 cpsc5300/data`

While you are running the SQL parser, you have the option to run professor's `test_heap_storage` function:

`$ SQL> test`

To run my tests on SlottedPage, HeapFile, and HeapTable from Milestone 2:

`$ make clean && make test && ./test`
