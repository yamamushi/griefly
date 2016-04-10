#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define size 30
#define source_step 5
#define blackhole_step 3

#define amount_t int

typedef struct _cell {
    amount_t amount;
    char in;
    char out;
    bool solid;
    char wave;
} cell;

typedef enum {
    LEFT = 1,
    RIGHT = 2,
    UP = 4,
    DOWN = 8,
} direction;

bool can_flow(char wave, direction dir) {
    switch (dir) {
    case LEFT:
        return (wave & RIGHT) == 0;
    case RIGHT:
        return (wave & LEFT) == 0;
    case UP:
        return (wave & DOWN) == 0;
    case DOWN:
        return (wave & UP) == 0;
    }

    return false;
}

direction opposite(direction dir) {
    switch (dir) {
    case LEFT:
        return RIGHT;
    case RIGHT:
        return LEFT;
    case UP:
        return DOWN;
    case DOWN:
        return UP;
    }

    return LEFT;
}

cell* cell_at(cell* field, int x, int y) {
    return &(field[size * x + y]);
}

cell* make_field() {
    int memsize = size*size*sizeof(cell);
    cell* field = (cell*) malloc(memsize);
    memset(field, 0, memsize);
    return field;
}

void prepare_edge(cell* current, cell* neigh, char* in, char* out) {
    if (neigh->solid) {
        return;
    }

    if (neigh->amount > current->amount) {
        ++(*in);
    } else if (neigh->amount < current->amount) {
        ++(*out);
    }
}

void prepare_cell_flow(cell* field, int xi, int yi) {
    cell* current = cell_at(field, xi, yi);
    if (current->solid) {
        return;
    }

    current->in = 0;
    current->out = 0;
    prepare_edge(current, cell_at(field, xi-1, yi), &current->in, &current->out);
    prepare_edge(current, cell_at(field, xi+1, yi), &current->in, &current->out);
    prepare_edge(current, cell_at(field, xi, yi-1), &current->in, &current->out);
    prepare_edge(current, cell_at(field, xi, yi+1), &current->in, &current->out);
}

void prepare_flows(cell* field) {
    for (int xi = 1; xi < size-1; ++xi) {
        for (int yi = 1; yi < size-1; ++yi) {
            prepare_cell_flow(field, xi, yi);
        }
    }
}

char cmax(char left, char right) {
    return left > right ? left : right;
}

amount_t calculate_delta(cell* left, cell* right, cell* lnext, cell* rnext, direction dir) {
    amount_t delta = (left->amount - right->amount) / cmax(left->out + 1, right->in);
    if (delta != 0) {
        lnext->wave |= dir;
        rnext->wave |= dir;
    }
    
    if (delta == 0 && lnext->amount > 0 && can_flow(left->wave, dir)) {
        delta = 1;
        lnext->wave |= dir;
        rnext->wave |= dir;
    }

    return delta;
}

void print_wave(char);

void run_edge_flow(cell* current, cell* next, cell* neigh, cell* neigh_next, direction dir) {
    if (neigh->solid) {
        return;
    }

    amount_t delta = 0;
    if (current->amount > neigh->amount) {
        // outbound
        delta = calculate_delta(current, neigh, next, neigh_next, dir);
    } else if (current->amount < neigh->amount) {
        // inbound
        delta = -calculate_delta(neigh, current, neigh_next, next, opposite(dir));
    }

    if (delta == 0) {
        return;
    }

    next->amount -= delta;
    neigh_next->amount += delta;
}

void run_cell_flow(cell* front, cell* back, int xi, int yi) {
    cell* current = cell_at(front, xi, yi);
    cell* next = cell_at(back, xi, yi);

    if (current->solid) {
        return;
    }

    // actually directions are linked to visualisation
    run_edge_flow(current, next, cell_at(front, xi+1, yi), cell_at(back, xi+1, yi), RIGHT);
    run_edge_flow(current, next, cell_at(front, xi, yi+1), cell_at(back, xi, yi+1), DOWN);
}

void run_flows(cell* front, cell* back) {
    for (int xi = 1; xi < size-1; ++xi) {
        for (int yi = 1; yi < size-1; ++yi) {
            run_cell_flow(front, back, xi, yi);
        }
    }
}

void print_field(cell* field);

void copy_field(cell* src, cell* dst) {
    int memsize = size*size*sizeof(cell);
    memcpy(dst, src, memsize);
}

void cleanup_wave(cell* field) {
    for (int xi = 1; xi < size-1; ++xi) {
        for (int yi = 1; yi < size-1; ++yi) {
            cell_at(field, xi, yi)->wave = 0;
        }
    }
}

void update_field(cell* field) {
    // blackholes
    for (int xi = blackhole_step; xi < size-1; xi += blackhole_step) {
        for (int yi = blackhole_step; yi < size-1; yi += blackhole_step) {
            cell_at(field, xi, yi)->amount = 0;
        }
    }

    // regenerators
    for (int xi = source_step; xi < size-1; xi += source_step) {
        for (int yi = source_step; yi < size-1; yi += source_step) {
            cell_at(field, xi, yi)->amount = 1000;
        }
    }
}

void run_simulation(cell* front, cell* back) {
    cell* tmp;

    for (int i = 0; i < 1000; ++i) {

        prepare_flows(front);
        cleanup_wave(back);

        print_field(front);
        getchar();

        run_flows(front, back);

        tmp = front;
        front = back;
        back = tmp;

        update_field(front);
        copy_field(front, back);
    }
}

void fill_field(cell* field) {
    // make borders solid
    for (int i = 0; i < size; ++i) {
        cell_at(field, i, 0)->solid = true;
        cell_at(field, i, size)->solid = true;
        cell_at(field, 0, i)->solid = true;
        cell_at(field, size, i)->solid = true;
    }

    // add atmos at every 5 tile
    for (int xi = source_step; xi < size-1; xi += source_step) {
        for (int yi = source_step; yi < size-1; yi += source_step) {
            cell_at(field, xi, yi)->amount = 1000;
        }
    }
}

int rows, cols;

void print_wave(char wave) {
    char* wave_str = (char*) malloc(6);
    memset(wave_str, ' ', 5);
    wave_str[5] = 0;

    if (wave & LEFT)
        wave_str[0] = '<';

    if (wave & DOWN)
        wave_str[1] = 'v';

    wave_str[2] = '.';

    if (wave & UP)
        wave_str[3] = '^';

    if (wave & RIGHT)
        wave_str[4] = '>';

    printf(" %s", wave_str);
}

void print_field(cell* field) {
    for (int yi = 0; yi < rows && yi < size; ++yi) {
        for (int xi = 0; xi < cols/5 && xi < size; ++xi) {
            printf(" %4d", cell_at(field, xi, yi)->amount);
            //print_wave(cell_at(field, xi, yi)->wave);
        }
        printf("\n");
    }    
}

int main(int argc, char** argv) {
    rows = atoi(getenv("LINES"));
    cols = atoi(getenv("COLUMNS"));

    if (rows == 0 || cols == 0) {
        printf("cannot get terminal resolution");
        return 1;
    }

    cell* field1 = make_field();
    cell* field2 = make_field();

    fill_field(field1);
    copy_field(field1, field2);

    run_simulation(field1, field2);
    return 0;
}
