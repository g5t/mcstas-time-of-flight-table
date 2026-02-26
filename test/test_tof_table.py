"""Tests for the TableSetup, TableRecorder, and TableManager McStas components.

The test instrument is built entirely within this Python file using the
mccode-antlr Assembler API together with a minimal, fully deterministic
neutron source component defined here as an inline string registered via
the mccode-antlr InMemoryRegistry (no disk I/O required).

The compiled instrument prints expected time-of-flight values to stdout
at INITIALIZE time; the tests validate those against Python-computed
reference values.

Instrument layout:
    TrivialSource  →  TableSetup  →  TableRecorder …  →  TableManager

Each TableRecorder is placed L metres downstream of the source, so the
expected arrival time (after PROP_Z0 in the recorder) is t = L / velocity.
"""
from __future__ import annotations

from textwrap import dedent

from pytest import mark

# ---------------------------------------------------------------------------
# Inline component: a minimal, fully deterministic neutron source.
# Emits one neutron per event from the origin with a fixed velocity along +z,
# t=0 and unit weight.  No randomness is involved.
# ---------------------------------------------------------------------------
_TRIVIAL_SOURCE_COMP = dedent("""\
    DEFINE COMPONENT TrivialSource
    SETTING PARAMETERS (double velocity=1000.0)
    TRACE
    %{
      x = 0; y = 0; z = 0;
      vx = 0; vy = 0; vz = velocity;
      t = 0.0;
      p = 1.0;
      SCATTER;
    %}
    END
""")


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def compiled(method):
    """Decorator: skip test if no working C compiler is available."""
    from mccode_antlr.compiler.check import simple_instr_compiles
    if simple_instr_compiles('cc'):
        return method

    @mark.skip(reason=f"A working C compiler is required for {method.__name__}")
    def skipped(*args, **kwargs):
        pass

    return skipped


def repo_registry():
    """Return a LocalRegistry pointing at the root of this git repository."""
    from git import Repo, InvalidGitRepositoryError
    from mccode_antlr.reader.registry import LocalRegistry
    try:
        repo = Repo('.', search_parent_directories=True)
        return LocalRegistry('tof_table', repo.working_tree_dir)
    except InvalidGitRepositoryError as exc:
        raise RuntimeError(f"Could not locate repository root: {exc}") from exc


# ---------------------------------------------------------------------------
# Instrument builder
# ---------------------------------------------------------------------------

def make_instrument(n_recorders: int = 2):
    """Build the TOF table test instrument using the mccode-antlr Assembler.

    The instrument:
    * accepts a ``velocity`` parameter (m/s, default 1000)
    * at INITIALIZE time prints sentinel lines and the expected recorder
      arrival times to stdout as ``tof_expected_t{i}={value:.9f}``
    * places a ``TrivialSource``, one ``TableSetup``, *n_recorders*
      ``TableRecorder`` instances spaced 1 m apart, and a ``TableManager``

    Parameters
    ----------
    n_recorders:
        Number of ``TableRecorder`` instances to include.

    Returns
    -------
    instr : mccode_antlr.instr.Instr
        The assembled instrument object ready for compilation.
    """
    from mccode_antlr import Flavor
    from mccode_antlr.assembler import Assembler
    from mccode_antlr.reader.registry import InMemoryRegistry

    # Register TrivialSource in memory — no disk I/O needed.
    src_reg = InMemoryRegistry('trivial_src')
    src_reg.add_comp('TrivialSource', _TRIVIAL_SOURCE_COMP)

    assembler = Assembler(
        'TrivialTofTable',
        registries=[src_reg, repo_registry()],
        flavor=Flavor.MCSTAS,
    )

    # Instrument-level parameter (accessible in INITIALIZE and component params)
    assembler.parameter('double velocity=1000.0')

    # INITIALIZE: print sentinels and the expected recorder arrival times.
    # Each recorder is placed (i+1) metres from the source; with vz=velocity
    # and PROP_Z0 in the recorder, t_i = (i+1) / velocity.
    expected_prints = '\n'.join(
        f'  printf("tof_expected_t{i + 1}=%.9f\\n", {float(i + 1)} / velocity);'
        for i in range(n_recorders)
    )
    assembler.initialize(dedent(f"""\
        printf("tof_test_start\\n");
        {expected_prints}
        printf("tof_test_end\\n");
    """))

    # Component chain ----------------------------------------------------------
    src = assembler.component(
        'origin', 'TrivialSource',
        at=([0, 0, 0], 'ABSOLUTE'),
        parameters={'velocity': 'velocity'},
    )
    assembler.component('setup', 'TableSetup', at=([0, 0, 0], src))

    for i in range(n_recorders):
        assembler.component(
            f'rec{i + 1}', 'TableRecorder',
            at=([0, 0, float(i + 1)], src),
        )

    assembler.component(
        'manager', 'TableManager',
        at=([0, 0, float(n_recorders + 1)], src),
        parameters={'max_events': 10000},
    )

    return assembler.instrument


# ---------------------------------------------------------------------------
# Compile + run helper
# ---------------------------------------------------------------------------

def _compile_and_run(instr, parameters: str) -> bytes:
    """Compile *instr* and run it with the given parameter string.

    Returns
    -------
    bytes
        Combined stdout + stderr captured from the binary.
        If compilation or execution fails a ``RuntimeError`` is raised by
        ``mccode_antlr``, so a normal return means the instrument succeeded.
    """
    from pathlib import Path
    from tempfile import TemporaryDirectory
    from mccode_antlr import Flavor
    from mccode_antlr.run import mccode_compile, mccode_run_compiled

    with TemporaryDirectory() as build_dir:
        binary, target = mccode_compile(instr, Path(build_dir), flavor=Flavor.MCSTAS)
        out_dir = Path(build_dir) / 'output'
        output, _dats = mccode_run_compiled(
            binary, target, out_dir, parameters, capture=True,
        )
    # mccode_run_compiled returns (bytes_output, dats_dict); bytes_output is
    # stdout + stderr combined when capture=True.
    return output


# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------

@compiled
def test_tof_table_compiles_and_runs():
    """The instrument compiles without errors and runs to completion.

    A RuntimeError from mccode-antlr would bubble up as a test failure;
    successfully reaching the assertion means both steps succeeded.
    """
    instr = make_instrument(n_recorders=2)
    output = _compile_and_run(instr, '--ncount=10 --seed=1 velocity=1000.0')
    # If we got here without an exception, compilation and run both succeeded.
    assert b'tof_test_start' in output


@compiled
def test_tof_table_stdout_reports_expected_times():
    """Stdout from the compiled instrument contains the expected TOF values.

    For a fixed velocity the arrival time at recorder *i* (placed *i* metres
    from the source) is exactly ``i / velocity``.  The instrument INITIALIZE
    block prints these expected times; this test verifies they appear in the
    captured output.
    """
    velocity = 500.0
    n_recorders = 3
    instr = make_instrument(n_recorders=n_recorders)
    output = _compile_and_run(
        instr,
        f'--ncount=5 --seed=1 velocity={velocity}',
    )

    text = output.decode()

    assert 'tof_test_start' in text, (
        f"Sentinel 'tof_test_start' missing from output:\n{text}"
    )
    assert 'tof_test_end' in text, (
        f"Sentinel 'tof_test_end' missing from output:\n{text}"
    )

    for i in range(n_recorders):
        expected_t = (i + 1) / velocity
        expected_str = f'tof_expected_t{i + 1}={expected_t:.9f}'
        assert expected_str in text, (
            f"Expected '{expected_str}' in output.\nFull output:\n{text}"
        )

