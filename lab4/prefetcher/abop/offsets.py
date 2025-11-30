"""Prepare array containing the offsets whose prime factorization contains no prime $>5$"""


def largest_prime_factor(n):
    i = 2
    while i * i <= n:
        if n % i:
            i += 1
        else:
            n //= i
    return n


candidates = []

for i in range(1, 256 + 1):
    if largest_prime_factor(i) <= 5:
        candidates.append(i)

print(candidates)
print(len(candidates))
