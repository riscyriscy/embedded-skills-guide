#ifndef REQUIRE_H
#define REQUIRE_H

/* REQUIRE checks a precondition that only a programming mistake can break. The check
   stays in every build. What happens on a failure is a policy that the target chooses
   by linking one definition of require_failed. */
void require_failed(const char *file, int line, const char *expr)
    __attribute__((noreturn));

#define REQUIRE(cond) ((cond) ? (void)0 : require_failed(__FILE__, __LINE__, #cond))

#endif
