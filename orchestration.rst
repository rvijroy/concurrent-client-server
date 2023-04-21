
STEP                               | actor  | responder | read_sem | write_sem      | waiting for? | 
-----------------------------------|--------|-----------|----------|--------------- |---------------
0. read data from user                | client | client    |    0     |     1          | user input
0. create request                     | client | server    |  0->1    |  1 -> 0        | write
1. service request & write response   | server | client    |  1->0    |  0 -> 1        | read
2. read response                      | client | client    |    0     |  1 -> 0 -> 1   | write