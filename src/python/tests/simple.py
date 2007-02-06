import sam

class SimpleSam(sam.ExecutionState):
    def __init__(self, samfile):
	sam.ExecutionState.__init__(self, samfile)

	self.stack = []
	self.heap = []

	for stack_changes, heap_changes in self:
	    for ch in stack_changes:
		self.buffer_stack_change(ch)
	    for ch in heap_changes:
		self.buffer_heap_change(ch)
	    self.commit_changes()

    def buffer_stack_change(self, change):
	pass

    def buffer_heap_change(self, change):
	pass

    def commit_changes(self):
	pass

if __name__ == '__main__':
    import sys
    ss = SimpleSam(sys.argv[1])
