import sys

temp_instructions = []
final_instructions = []
memory = []
memo = {}
labels = {}
memo_counter = 0
with open(sys.argv[1], 'r', encoding='utf-8') as f:
    for line in f:
        content = line.strip()
        if content == "" or content.startswith("#"): pass
        elif content.startswith("."): 
            words = content.split()
            memo[words[0][1:-1]]=memo_counter
            values = [int(x) for x in words[1:]]
            memory.extend(values)
            memo_counter+=len(values)
        else : temp_instructions.append(content)    

for i in range(len(temp_instructions)) :
    if temp_instructions[i].find(':')==-1: final_instructions.append(temp_instructions[i])
    else:
        s = temp_instructions[i]
        ind = s.find(':')
        if ind+1!=len(s): 
            labels[s[0:ind]] = len(final_instructions) 
            final_instructions.append(s[ind+1:].strip())
        else: labels[s[0:ind]] = len(final_instructions)

for i in range(len(final_instructions)) :
    s = final_instructions[i]
    instruction = [x.strip() for x in s.split(',')]
    parts = instruction[0].split()
    if parts[0] == 'j' and parts[1] in labels:
        parts[1] = str(labels[parts[1]] - i)
        instruction[0] = ' '.join(parts)
    elif instruction[-1] in labels:
        #branch instruction
        instruction[-1] = str(labels[instruction[-1]]-i)
    elif instruction[-1].find('(')!=-1:
        #memory related instruction
        if instruction[-1][:instruction[-1].find('(')] not in memo:pass
        else: 
            ind = instruction[-1].find('(')
            cur_label = instruction[-1][:ind]
            instruction[-1] = str(memo[cur_label]) + instruction[-1][ind:]
    final_instructions[i] = ', '.join(instruction)


with open(sys.argv[1], 'w') as f:
    if memory:
        f.write('.DATA ' + ' '.join(str(x) for x in memory) + '\n')
    for inst in final_instructions:
        f.write(inst + '\n')