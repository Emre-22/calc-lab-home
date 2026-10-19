# Emre Elaziz — 251ADB139

The graded program is `calc.c`. All parser, arithmetic, file handling, and CLI
code is in that file. Output names use `emre_elaziz_251ADB139`.

## Build and run on Linux / Codespaces

```sh
gcc -O2 -Wall -Wextra -std=c17 -o calc calc.c -lm
./calc -o results input.txt
./calc -d calc-tests/tests -o results
sh calc-tests/run_tests.sh ./calc
sh calc-tests/cli.sh ./calc
python3 extra_tests.py ./calc
```

Directory mode processes all direct regular `.txt` files, including any
`_output.txt` fixtures if you point it at the sample tests folder. Use the
supplied test runner to compare fixtures with their expected results.

```sh
gcc -O1 -g -Wall -Wextra -std=c17 -fsanitize=address,undefined -o calc calc.c -lm
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 sh calc-tests/run_tests.sh ./calc
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 sh calc-tests/cli.sh ./calc
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 python3 extra_tests.py ./calc
```

The included GitHub Actions workflow runs both builds and all three suites on
Ubuntu after a push. The workflow has been prepared but has not run here.

## Verified here (19 October 2026)

- GCC 16.1.0, Windows w64devkit: the exact optimized flags compile without warnings.
- Original `run_tests.sh`: 44 passed, 0 failed.
- Original `cli.sh`: all 12 checks passed.
- `extra_tests.py`: 38 expression/stress cases and 8 additional CLI checks passed.
- Stress cases include a 3 MB single expression, one million expression lines,
  a 5 MB comment, a late-file error, 1000 nested parentheses, and 100000 powers.

The requested sanitizer build was attempted. This installed Windows GCC lacks
`libasan` and `libubsan` and cannot link it. WSL access is denied and no Docker
engine is running. Therefore sanitizer execution is **not verified here**;
run the commands above in Codespaces or check the included Ubuntu workflow.
No claim of a guaranteed grade is made.

## How to explain the program

- Tokenization happens inside `evaluate`: a number consumes only the permitted
  digits or digits-dot-digits shape. Exponents such as `1.5e3` are rejected.
- Every line has an absolute byte offset. Adding its local token index and one
  produces the required position. Newlines and ignored comments still count.
- Dynamic value and operator stacks implement precedence without recursion.
  Equal precedence reduces immediately except for `**`, which waits and thus
  evaluates from the right. Parentheses act as reduction barriers.
- Each value retains its operand start. A grouped value starts at its opening
  parenthesis, making division-by-zero and negative-exponent positions correct.
- Two integers use `long long`; any float operand selects `double`. Integer
  power uses repeated squaring. Floating results must remain finite.
- Line and stack buffers double in capacity, avoiding repeated whole-buffer
  copying. File processing is linear apart from logarithmic integer powers.
- Successful lines are written to a temporary stream. The final destination
  receives either all results or only the first `ERROR`, never partial results.
- The CLI snapshots directory inputs before processing, ignoring subdirectories
  and non-`.txt` entries, and handles I/O failures with stderr and failure status.

Input is read in binary mode so physical characters determine positions. The
specified whitespace between tokens is space/tab; line endings in the supplied
fixtures and additional tests are LF. Integer arithmetic assumes the assignment's
no-overflow contract.

## Submission steps still requiring your GitHub account

1. Read `calc.c` and `AI_HISTORY.md`; make sure you can explain the implementation.
2. Create your repository on GitHub and open its Codespace, as the assignment asks.
3. Place these files at its top level, including `.github/workflows/test.yml`.
4. Run both builds and test suites above; inspect the sanitizer workflow result.
5. Commit and push real changes as you work. Do not invent an earlier history.
6. Submit the URL of the last pushed commit, in the assignment's required form:
   `https://github.com/<username>/<repository>/commit/<full-hash>`.

GitHub CLI authentication for the existing local account was invalid during
this session. No remote repository, Codespace, push, or submission link was created.

## Resources

`AI_HISTORY.md` records the user prompts, assistant messages, assistance scope,
and verification history. `ASSIGNMENT.txt` is the supplied specification;
`calc-tests/` contains the original supplied scripts and fixtures unchanged.
Python is used only by additional tests; the C calculator never invokes it.
