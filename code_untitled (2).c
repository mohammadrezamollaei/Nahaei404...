 #include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#define MAX_FORMULA 256

/*Error*/
typedef enum 
{
    ERR_NONE,
    ERR_DIV_ZERO,
    ERR_UNKNOWN_FUNC,
    ERR_INVALID_CELL,
    ERR_PAREN,
    ERR_DOMAIN
} ErrorType;

/*Cell*/
typedef struct 
{
    double value;
    char formula[MAX_FORMULA];
} Cell;

/*Sheet*/
int ROWS = 10;
int COLS = 10;
Cell **sheet;

/*Parser*/
const char *expr;
ErrorType errorType;

/*Functions*/
void skipSpaces();
void initSheet(int r, int c);
void expandSheet();
void showSheet();

double getCellValue(const char *ref);
double parseExpression();
double parseTerm();
double parseFactor();
void evaluateCell(int r, int c, const char *f);

void printError(ErrorType e);

void printCell(const char *addr);
void saveSheet();
void loadSheet();

void sortRow(int row, int c1, int c2);
void sortColumn(int col, int r1, int r2);
void sortCommand();

/*Main*/
int main() 
{
    char cmd[10], addr[4], formula[MAX_FORMULA];
    initSheet(ROWS, COLS);

    while (1) 
    {
        printf("\nCommand [show/set/print/save/load/expand/sort/exit]: ");
        scanf("%9s", cmd);
        getchar();

        if (!strcmp(cmd, "show")) showSheet();

        else if (!strcmp(cmd, "set")) 
        {
            printf("Cell: ");
            scanf("%3s", addr);
            getchar();

            int c = toupper(addr[0]) - 'A';
            int r = atoi(addr + 1) - 1;

            if (r < 0 || r >= ROWS || c < 0 || c >= COLS) 
            {
                printf("Error: Out of bounds\n");
                continue;
            }

            printf("Formula: ");
            fgets(formula, MAX_FORMULA, stdin);
            formula[strcspn(formula, "\n")] = 0;
            evaluateCell(r, c, formula);
        }

        else if (!strcmp(cmd, "print")) 
        {
            printf("Cell: ");
            scanf("%3s", addr);
            printCell(addr);
        }
        else if (!strcmp(cmd, "save")) saveSheet();
        else if (!strcmp(cmd, "load")) loadSheet();
        else if (!strcmp(cmd, "expand")) expandSheet();
        else if (!strcmp(cmd, "sort")) sortCommand();
        else if (!strcmp(cmd, "exit")) break;
    }
    return 0;
}

/* IMPLEMENTATIONS */

void skipSpaces() 
{
    while (*expr == ' ') expr++;
}

void initSheet(int r, int c) 
{
    sheet = malloc(r * sizeof(Cell *));
    for (int i = 0; i < r; i++) {
        sheet[i] = malloc(c * sizeof(Cell));
        for (int j = 0; j < c; j++) {
            sheet[i][j].value = 0;
            sheet[i][j].formula[0] = '\0';
        }
    }
}

void expandSheet() 
{
    int addR, addC;
    printf("Add rows: ");
    scanf("%d", &addR);
    printf("Add cols: ");
    scanf("%d", &addC);

    if (addR <= 0 && addC <= 0) 
    {
        printf("Nothing to expand.\n");
        return;
    }

    int newR = ROWS + addR;
    int newC = COLS + addC;

    Cell **newSheet = malloc(newR * sizeof(Cell *));
    for (int i = 0; i < newR; i++) 
    {
        newSheet[i] = malloc(newC * sizeof(Cell));
        for (int j = 0; j < newC; j++) 
        {
            if (i < ROWS && j < COLS)
                newSheet[i][j] = sheet[i][j];
            else 
            {
                newSheet[i][j].value = 0;
                newSheet[i][j].formula[0] = '\0';
            }
        }
    }

    for (int i = 0; i < ROWS; i++) free(sheet[i]);
    free(sheet);

    sheet = newSheet;
    ROWS = newR;
    COLS = newC;

    printf("Table expanded to %dx%d\n", ROWS, COLS);
}

double getCellValue(const char *ref) 
{
    int col = toupper(ref[0]) - 'A';
    int row = atoi(ref + 1) - 1;
    if (row < 0 || row >= ROWS || col < 0 || col >= COLS) 
    {
        errorType = ERR_INVALID_CELL;
        return 0;
    }
    return sheet[row][col].value;
}

double parseFactor() 
{
    skipSpaces();
    if (*expr == '-') 
    {
        expr++;
        return -parseFactor();
    }
    if (*expr == '(') 
    {
        expr++;
        double v = parseExpression();
        skipSpaces();
        if (*expr != ')') 
        {
            errorType = ERR_PAREN;
            return 0;
کوروش قلعه نووی: }
        expr++;
        return v;
    }
    if (isdigit(*expr) || *expr == '.')
        return strtod(expr, (char **)&expr);

    if (isalpha(*expr)) 
    {
        char name[16] = {0};
        int i = 0;
        while (isalpha(*expr)) name[i++] = *expr++;

        if (*expr == '(') 
        {
            expr++;
            double a = parseExpression(), b = 0;
            skipSpaces();
            if (*expr == ',') 
            {
                expr++;
                b = parseExpression();
            }
            skipSpaces();
            if (*expr != ')') 
            {
                errorType = ERR_PAREN;
                return 0;
            }
            expr++;

            if (!strcmp(name, "sin")) return sin(a);
            if (!strcmp(name, "cos")) return cos(a);
            if (!strcmp(name, "tan")) return tan(a);
            if (!strcmp(name, "sinh")) return sinh(a);
            if (!strcmp(name, "cosh")) return cosh(a);
            if (!strcmp(name, "tanh")) return tanh(a);
            if (!strcmp(name, "sqrt")) { if (a < 0) { errorType = ERR_DOMAIN; return 0; } return sqrt(a); }
            if (!strcmp(name, "ln") || !strcmp(name, "log")) { if (a <= 0) { errorType = ERR_DOMAIN; return 0; } return log(a); }
            if (!strcmp(name, "exp")) return exp(a);
            if (!strcmp(name, "abs")) return fabs(a);
            if (!strcmp(name, "pow")) return pow(a, b);

            errorType = ERR_UNKNOWN_FUNC;
            return 0;
        }

        char ref[8] = {0};
        ref[0] = name[0];
        int j = 1;
        while (isdigit(*expr)) ref[j++] = *expr++;
        return getCellValue(ref);
    }

    errorType = ERR_PAREN;
    return 0;
}

double parseTerm() 
{
    double v = parseFactor();
    skipSpaces();
    while (*expr == '*' || *expr == '/') 
    {
        char op = *expr++;
        double r = parseFactor();
        if (op == '*') v *= r;
        else 
        {
            if (r == 0) 
            {
                errorType = ERR_DIV_ZERO;
                return 0;
            }
            v /= r;
        }
        skipSpaces();
    }
    return v;
}

double parseExpression() 
{
    double v = parseTerm();
    skipSpaces();
    while (*expr == '+' || *expr == '-') 
    {
        char op = *expr++;
        double r = parseTerm();
        if (op == '+') v += r;
        else v -= r;
        skipSpaces();
    }
    return v;
}

void printError(ErrorType e) 
{
    printf("Error: ");
    switch (e) {
        case ERR_DIV_ZERO: printf("Division by zero\n"); break;
        case ERR_UNKNOWN_FUNC: printf("Unknown function\n"); break;
        case ERR_INVALID_CELL: printf("Out of bounds\n"); break;
        case ERR_PAREN: printf("Mismatched parentheses\n"); break;
        case ERR_DOMAIN: printf("Math domain error\n"); break;
        default: break;
    }
}

void evaluateCell(int r, int c, const char *f) 
{
    expr = f;
    errorType = ERR_NONE;
    double res = parseExpression();
    if (errorType != ERR_NONE || isnan(res) || isinf(res)) 
    {
        printError(errorType);
        return;
    }
    sheet[r][c].value = res;
    strcpy(sheet[r][c].formula, f);
}

void printCell(const char *addr) 
{
    int col = toupper(addr[0]) - 'A';
    int row = atoi(addr + 1) - 1;
    if (row < 0 || row >= ROWS || col < 0 || col >= COLS) 
    {
        printf("Error: Out of bounds\n");
        return;
    }
    printf("Cell %s\nValue: %.6f\nFormula: %s\n",
           addr,
           sheet[row][col].value,
           strlen(sheet[row][col].formula) ? sheet[row][col].formula : "(none)");
}

void saveSheet() 
{
    char name[64];
    printf("File name: ");
    scanf("%63s", name);
    FILE *fp = fopen(name, "w");
    if (!fp) return;
    for (int r = 0; r < ROWS; r++)
        for (int c = 0; c < COLS; c++)
            fprintf(fp, "%d %d %.10lf %s\n", r, c, sheet[r][c].value, sheet[r][c].formula);
    fclose(fp);
}

void loadSheet() 
{
    char name[64];
    printf("File name: ");
    scanf("%63s", name);
    FILE *fp = fopen(name, "r");
    if (!fp) return;
    int r, c;
    double v;
    char f[MAX_FO
کوروش قلعه نووی: RMULA];
    while (fscanf(fp, "%d %d %lf %[^\n]\n", &r, &c, &v, f) == 4) 
    {
        if (r < ROWS && c < COLS) 
        {
            sheet[r][c].value = v;
            strcpy(sheet[r][c].formula, f);
        }
    }
    fclose(fp);
}

void showSheet() 
{
    printf("\n     ");
    for (int c = 0; c < COLS; c++)
        printf("%8c", 'A' + c);
    printf("\n");
    for (int r = 0; r < ROWS; r++) 
    {
        printf("%4d ", r + 1);
        for (int c = 0; c < COLS; c++)
            printf("%8.2f", sheet[r][c].value);
        printf("\n");
    }
}

void sortRow(int row, int c1, int c2) 
{
    for (int i = c1; i < c2; i++)
        for (int j = i + 1; j <= c2; j++)
            if (sheet[row][i].value > sheet[row][j].value) 
            {
                double tmp = sheet[row][i].value;
                sheet[row][i].value = sheet[row][j].value;
                sheet[row][j].value = tmp;
            }
}

void sortColumn(int col, int r1, int r2) 
{
    for (int i = r1; i < r2; i++)
        for (int j = i + 1; j <= r2; j++)
            if (sheet[i][col].value > sheet[j][col].value) 
            {
                double tmp = sheet[i][col].value;
                sheet[i][col].value = sheet[j][col].value;
                sheet[j][col].value = tmp;
            }
}

void sortCommand() 
{
    int choice;
    printf("1. Sort Row\n2. Sort Column\nChoice: ");
    scanf("%d", &choice);

    if (choice == 1) 
    {
        int row;
        char fromC, toC;
        printf("Row number: ");
        scanf("%d", &row);
        printf("From column: ");
        scanf(" %c", &fromC);
        printf("To column: ");
        scanf(" %c", &toC);
        row--;
        int c1 = toupper(fromC) - 'A';
        int c2 = toupper(toC) - 'A';
        if (row < 0 || row >= ROWS || c1 < 0 || c2 >= COLS || c1 > c2) 
        {
            printf("Error: Out of bounds\n");
            return;
        }
        sortRow(row, c1, c2);
    }
     else if (choice == 2) 
    {
        char colC;
        int r1, r2;
        printf("Column letter: ");
        scanf(" %c", &colC);
        printf("From row: ");
        scanf("%d", &r1);
        printf("To row: ");
        scanf("%d", &r2);
        int col = toupper(colC) - 'A';
        r1--; r2--;
        if (col < 0 || col >= COLS || r1 < 0 || r2 >= ROWS || r1 > r2) {
            printf("Error: Out of bounds\n");
            return;
        }
        sortColumn(col, r1, r2);
    }
}