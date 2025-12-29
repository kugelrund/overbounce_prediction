# Overbounce Prediction

Code for predicting the probability of an overbounce in games that use the
movement code of the id Tech 3 engine. This was written for the three
**id Tech 3** games by Raven Software, *Star Trek: Voyager - Elite Force*,
*Star Wars Jedi Knight II: Jedi Outcast* and
*Star Wars Jedi Knight: Jedi Academy*, but it should work fine for other
**id Tech 3** games too.

## What is an overbounce?

See Matt's Ramblings excellent [YouTube video](https://www.youtube.com/watch?v=WmO2cdTU7EM) explanation.

In a nutshell, if the player's position ends up `0.25` units above the actual ground in a frame, a special check in the code will already consider them to be on the ground, without having deduced their vertical speed due to the collision.
The remaining vertical speed is then translated into a new movement direction.

## How is the prediction made?

Assuming the player is in free fall at a current frame $`i`$, the next vertical velocity $`v_{i+1}`$ and z-coordinate $`z_{i+1}`$ are given by

```math
\begin{align*}
v_{i+1} &= v_i - g \Delta t_{i+1} \\
z_{i+1} &= z_i + \frac{v_i + v_{i+1}}{2} \Delta t_{i+1}
\end{align*}
```

where $`\Delta t_{i+1}`$ is the frametime between frame $i$ and frame $i+1$ that `Pmove` is executed with.
Let us write $t_i$ for the time passed between frame $0$ and frame $`i`$, i.e.
$`t_i := \sum_{j=1}^i \Delta t_j `$ for all $`i \geq 0`$.

Given an initial z-coordinate $`z_0`$ and an initial vertical velocity $`v_0`$, a proof by induction leads to the explicit formulas

```math
\begin{align*}
v_i &= v_0 - g t_i \\
z_i &= z_0 + v_0 t_i - \frac{g}{2} t_i^2 \text{.}
\end{align*}
```

This means that given the initial height $`z_0`$ and vertical speed $`v_0`$, the speed and position at every frame of the free fall only depend on the cumulative frametime $`t_i`$ up to that point.
The exact frametimes of the single frames is not relevant.
So by creating a statistic of the cumulative frametime of consecutive frames, the probability of certain cumulative frametimes can be estimated.
And with that, the probability of heights $`z_i`$ that are right in that window of `0.25` units above the ground can be estimated, giving an estimate for the probability of an overbounce at abritary heights.

Note that if playing with `pmove_fixed 1` in Quake 3 Arena, the frametime for `Pmove` is always a constant `pmove_msec`.
That means that th only possible cumulative frametimes $`t_i`$ are always multiples of `pmove_msec`, making overbounce prediction entirely deterministic.
Raven's Quake 3 engine games do not have `pmove_fixed`, meaning that frametimes are not consistent.
The prediction can therefore only make probabilistic estimates using observed frametimes as described above.

### Proof for explicit formulas

The proof for $`v_i`$ is trivial, so left to show:

```math
z_i = z_0 + v_0 t_i - \frac{g}{2} t_i^2\text{.}
```

The base case for $`i=0`$ is clear since

```math
z_0 = z_0 + v_0 \cdot 0 - \frac{g}{2} \cdot 0^2 = z_0 + v_0 t_0 - \frac{g}{2} t_0^2 \text{.}
```

So assume the identity holds for an arbitrary $`i \geq 0`$.
Then

```math
\begin{align*}
z_{i+1} = z_i + \frac{v_i + v_{i+1}}{2} \Delta t_{i+1} &= z_0 + v_0 t_i - \frac{g}{2} t_i^2 + \frac{v_i + v_{i+1}}{2} \Delta t_{i+1} \\
&= z_0 + v_0 t_i - \frac{g}{2} t_i^2 + \frac{v_0 - g t_i + v_0 - g t_{i+1}}{2} \Delta t_{i+1} \\
&= z_0 + v_0 t_{i+1} - \frac{g}{2} (t_i^2 + (t_i + t_{i+1}) \Delta t_{i+1}) \\
&= z_0 + v_0 t_{i+1} - \frac{g}{2} (t_i^2 + (2 t_i + \Delta t_{i+1}) \Delta t_{i+1}) \\
&= z_0 + v_0 t_{i+1} - \frac{g}{2} (t_i + \Delta t_{i+1})^2 \\
&= z_0 + v_0 t_{i+1} - \frac{g}{2} t_{i+1}^2
\end{align*}
```

which concludes the induction step.
