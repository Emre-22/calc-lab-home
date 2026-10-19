"""Additional black-box checks; run: python3 extra_tests.py ./calc"""
import pathlib
import subprocess
import sys
import tempfile
import time

calc = str(pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else './calc').resolve())
cases = [
    (b'', ''), (b' \t\n#comment\n\t# comment', ''),
    (b'7/2\n(0-7)/2\n7/(0-2)\n', '3\n-3\n-3\n'),
    (b'0**0\n2**3**2\n2*3**2+1\n', '1\n512\n19\n'),
    (b'0.5**(0-1)\n1.25*4\n0.1+0.2\n', '2\n5\n0.3\n'),
    (b'2 ** (1 - 2)', 'ERROR:6\n'),
    (b'10 / (3 - 3)', 'ERROR:6\n'),
    (b'1.5 / 0.0', 'ERROR:7\n'),
    (b'1/(0.0-0.0)', 'ERROR:3\n'),
    (b'(0-8)**0.5', 'ERROR:6\n'),
    (b'10.0 ** 400', 'ERROR:6\n'),
    (b'1\n2\n3 *\n', 'ERROR:8\n'),
    (b'1 + 2\n3 +', 'ERROR:10\n'),
    (b'# xyz\n \n3 *\n', 'ERROR:12\n'),
    (b'3 *\n', 'ERROR:4\n'),
    (b'(2 + 3\n', 'ERROR:7\n'),
    (b'(1 + 2))', 'ERROR:8\n'),
    (b'2 * ()', 'ERROR:6\n'),
    (b'-5', 'ERROR:1\n'), (b'2 * -3', 'ERROR:5\n'),
    (b'.5', 'ERROR:1\n'), (b'5.', 'ERROR:2\n'),
    (b'1.5e3', 'ERROR:4\n'), (b'1.2.3', 'ERROR:4\n'),
    (b'2 3', 'ERROR:3\n'), (b'2(3)', 'ERROR:2\n'),
    (b'2 * * 3', 'ERROR:5\n'), (b'2 *** 3', 'ERROR:5\n'),
    (b'1 #x', 'ERROR:3\n'), (b'1\x00+2', 'ERROR:2\n'),
    (b'9223372036854775807\n(0-9223372036854775807)-1',
     '9223372036854775807\n-9223372036854775808\n'),
    (b'1**9223372036854775807\n2**62\n', '1\n4611686018427387904\n'),
    (b'('*1000 + b'1' + b')'*1000, '1\n'),
    (b'1**'*100000 + b'1', '1\n'),
    (b'1+'*1500000 + b'1', '1500001\n'),
    (b'1+2\n'*1000000, '3\n'*1000000),
    (b'#'+b'x'*5000000+b'\n2+2', '4\n'),
    (b'1\n'*1000000+b'2+&', 'ERROR:2000003\n'),
]
start = time.monotonic()
with tempfile.TemporaryDirectory() as tmp:
    root = pathlib.Path(tmp)
    for index, (data, expected) in enumerate(cases):
        source = root / f'case{index}.txt'
        out = root / f'out{index}'
        source.write_bytes(data)
        run = subprocess.run([calc, '-o', str(out), str(source)], capture_output=True, timeout=60)
        assert run.returncode == 0, (index, run.returncode, run.stderr)
        assert not run.stderr, (index, run.stderr)
        files = list(out.glob('*.txt'))
        assert len(files) == 1, (index, files)
        actual = files[0].read_text()
        assert actual == expected, (index, actual[:100], expected[:100])
    # CLI failure paths absent from the supplied suite.
    blocked = root / 'blocked'
    blocked.write_text('not a directory')
    for args in [['-d'], ['-o'], ['-d', str(root), str(source)],
                 [str(source), str(source)], ['-o', str(blocked), str(source)],
                 ['-o', str(root/'missing'/'out'), str(source)], [str(root)]]:
        run = subprocess.run([calc, *args], capture_output=True, timeout=10)
        assert run.returncode != 0 and run.stderr, (args, run)
    # A directory named *.txt is ignored; output may equal input directory.
    same = root / 'same'
    same.mkdir()
    (same/'folder.txt').mkdir()
    (same/'a.txt').write_bytes(b'2+3\n')
    run = subprocess.run([calc, '-d', str(same), '-o', str(same)], capture_output=True, timeout=10)
    assert run.returncode == 0 and not run.stderr, run
    assert (same/'a_emre_elaziz_251ADB139.txt').read_text() == '5\n'
print(f'{len(cases)} expression/stress cases and 8 additional CLI checks passed ({time.monotonic()-start:.2f}s)')

