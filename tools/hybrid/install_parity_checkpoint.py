#!/usr/bin/env python3
"""Install opt-in checkpoint in a private, already hybrid-patched DOSBox-X tree."""
import argparse
from pathlib import Path
import shutil


def install(source):
    cpu = source / 'src/cpu'
    observer = cpu / 'pr_hybrid_observer.h'
    core = cpu / 'core_normal.cpp'
    obs, normal = observer.read_text(), core.read_text()
    if 'PR_ParityCheckpoint' in obs or 'parity_checkpoint.h' in normal:
        raise ValueError('checkpoint already installed')
    anchor = 'namespace pr_obs {'
    hit = '        ++e.count; ++arrivals;'
    include = '#include "pr_hybrid_dispatch.h"'
    if any(text.count(key) != 1 for text, key in
           [(obs, anchor), (obs, hit), (normal, include)]):
        raise ValueError('unexpected hybrid source; no files changed')
    obs = obs.replace(anchor, 'static void PR_ParityCheckpoint(const char *, uint64_t);\n' + anchor)
    obs = obs.replace(hit, hit + '\n        PR_ParityCheckpoint(e.id.c_str(), e.count);')
    normal = normal.replace(include, include + '\n#include "parity_checkpoint.h"')
    shutil.copyfile(Path(__file__).with_name('parity_checkpoint.h'), cpu / 'parity_checkpoint.h')
    observer.write_text(obs)
    core.write_text(normal)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    install(parser.parse_args().source)
