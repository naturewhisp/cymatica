# L3 (A-B-A alternation in chained phrases) and L5 (paths of 3 consecutive chained phrases, each with material new in
# its section), from the Vettore groups and the chain decisions of C5 and C5b. A phrase is chained when its base is
# the winner of the previous phrase and no lineage reset happened.
import re
def read(groups_file, chain_file):
    labels = list(map(int, re.search(r'labels \[(.*)\]', open(groups_file).read()).group(1).split(', ')))
    chained = {}
    for line in open(chain_file):
        m = re.match(r'P (\d+) src (-?\d+) reset (\d)', line.strip())
        if m:
            n, src = int(m.group(1)), int(m.group(2))
            chained[n] = src >= 0 and src == n - 1 and m.group(3) == '0'
    return labels, chained
for name, g, c in [('C5', 'docs/research/experiments/m25_3_c5_lineage/vettore_groups_fixture_intents_c5.output.txt', 'docs/research/experiments/m25_3_c5_lineage/chain_decisions_c5.output.txt'),
                   ('C5b', 'docs/research/experiments/m25_3_c5b_chain_ops/vettore_groups_c5b.output.txt', 'docs/research/experiments/m25_3_c5b_chain_ops/chain_decisions_c5b.output.txt')]:
    lab, ch = read(g, c)
    cand = [n for n in range(24) if ch.get(n) and n % 8 >= 2]
    alt = [n for n in cand if lab[n] == lab[n - 2] and lab[n] != lab[n - 1]]
    newin = lambda n: all(lab[j] != lab[n] for j in range((n // 8) * 8, n))
    paths = [n for n in range(22) if all(ch.get(k) and newin(k) and k // 8 == n // 8 for k in (n, n + 1, n + 2))]
    print(name, 'chained phrases with n-2 in the section', len(cand), '| A-B-A alternations', len(alt), alt,
          '| paths of 3 new chained phrases start at', paths)
