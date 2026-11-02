struct s {
    union {
        int word;
        char byte;
    };
};
int f(struct s *p)
{
    return p->word;
}
