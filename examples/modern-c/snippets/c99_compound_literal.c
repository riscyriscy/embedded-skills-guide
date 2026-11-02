struct s {
    int a;
};
int f(const struct s *p);
int g(void)
{
    return f(&(struct s){.a = 1});
}
