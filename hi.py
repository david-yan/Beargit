nodes = int(input())
tokens = [int(x) for x in input().split()]
ways = []
sums = [0]

for num in range(0, nodes-1):
    ways += [[int(x)-1 for x in input().split()]]
    sums += [0]

def sum_tree(root):
    if (sums[root] == 0):
        sum = tokens[root]
        for way in ways:
            if (way[0] == root):
                sum += sum_tree(way[1])
        sums[root] = sum
    return sums[root]

def max_score(root, top_score):
    branches = [(way[1], sum_tree(way[1])) for way in ways if way[0] == root]
    max = 0
    max_sum = 0
    for branch in branches:
        if (branch[1] > max_sum):
            max_sum = branch[1]
            max = branch[0]

    if (2 * max_sum > sum_tree(root) + top_score):
        return max_score(max, top_score + sum_tree(root) - max_sum)
    return top_score + sum_tree(root) - max_sum

print(max_score(0, 0))
