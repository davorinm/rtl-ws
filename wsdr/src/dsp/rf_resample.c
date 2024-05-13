// https://stackoverflow.com/questions/44668774/reduce-fractions-in-c

// gcf function - return gcd (greatest common divisor) of two numbers
int gcd(int n, int m)
{
    int gcd, remainder;

    while (n != 0)
    {
        remainder = m % n;
        m = n;
        n = remainder;
    }

    gcd = m;

    return gcd;
}

// reduced fraction:
int calc(int number1, int number2) {
    int newNumber1, newNumber2;

    //--get user input
    printf("Enter a fraction: ");
    scanf("%d/%d", &number1, &number2);

    //--calculations
    //find the gcd of numerator and denominator
    //then divide both the numerator and denominator by the GCD
    int val = gcd(number1, number2);
    newNumber1 = number1 / val;
    newNumber2 = number2 / val;

    //--results
    printf("In lowest terms: %d/%d %d", newNumber1, newNumber2, val);
}