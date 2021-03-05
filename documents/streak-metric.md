# Streak metric for tag-based referencing

For this work, we used the Streak metric to quantify the similarity between two tags when performing
a tag-based reference.
The Streak metric was originally proposed by [@downing_intelligence_2015]
At a high-level, this metric measures similarity between two bit strings in terms of the relationship between the lengths of the longest contiguously-matching and longest contiguously-mismatching substrings.
That is, we XOR the two bit strings and count the longest substring of all 0's to find the longest contiguously-matching substring and the longest substring of all 1's to find the longest contiguously-mismatching substring.
Our implementation of the Streak metric can be found in the Empirical library [@charles_ofria_2020_empirical].
We provide a formal definition of the Streak metric below.

## Formal definition

Each tag was represented as an ordered, fixed-length bit string,

$$ t = \langle t_0, t_1, t_2, \dots, t_{n-2}, t_{n-1} \rangle $$

where

$$ \forall i, t_i \in \{0, 1\}. $$

We define the greatest contiguously-matching length of $n$-long bitstrings $t$ and $u$ as follows,

$$ m(t, u) = \max(\{i - j \forall i, j \in 0..n-1 \mid \forall q \in i..j, t_q = u_q \}) $$

We define the greatest contiguously-mismatching length as follows,

$$ n(t, u) = \max(\{i - j \forall i, j \in 0..n-1 \mid \forall q \in i..j, t_q \neq u_q \}) $$


The streak metric $S'$  with tags $t$ and $u$

$$
S'(t, u)
= \frac{ p(n(t,u)) }{p(m(t,u)) + p(n(t,u))}.
$$

where $p$ approximates the probability of a contiguously-matching substring of a given length between $t$ and $u$.

It is worth noting that the formula for computing the probability of a $k$-bit match or mismatch, given by Downing as follows, is actually mathematically flawed.

$$
p_k
= \frac{n - k + 1}{2^k}
$$

The probability of a $0$-bit match according to this formula would be computed as $p_0 = \frac{n - 0 + 1}{2^0} = n + 1$ which is clearly impossible because $p_0 > 1 \forall n > 0$.
The actual probability be computed using a lookup table computed using dynamic programming.
However, the formula Downing presented provides a useful approximation to the probability of a $k$ bit match.
For computational efficiency and consistency with the existing literature we use clamp edge cases between 0 and 1 to yield the corrected streak metric $S$.

$$
S(t, u) =
\max( \min( S'(t, u), 1), 0)
$$