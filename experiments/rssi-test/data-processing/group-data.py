import itertools

# https://napsterinblue.github.io/notes/python/internals/itertools_sliding_window/
def sliding_window(iterable, n=2):
    iterables = itertools.tee(iterable, n)
    for iterable, num_skipped in zip(iterables, itertools.count()):
        for _ in range(num_skipped):
            next(iterable, None)
    return zip(*iterables)

def parse_line(line):
  return list(map(lambda s: int(s), line.strip().split('\t')))

def parse_file(file_name):
  f = open(file_name)
  lines = f.readlines()[7:]
  return [parse_line(line) for line in lines]

def group_data(data):
  groups = [[data[0]]]

  for prev, next in sliding_window(data):
    if next[1] < prev[1]:
      groups.append([])
    groups[-1].append(next)

  return groups

data = parse_file("../data/sw.txt")
groups = group_data(data)
