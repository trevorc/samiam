import sambase

class ExecutionState(sambase.ExecutionStateBase):
    class Iter:
	def __init__(self, es):
	    self.__es = es
	    self.__err = sam

        def __iter__(self):
            return self

        def next(self):
	    if (self.es.pc() > len(self.es.instructions()) or
		self.__err != ExecutionState.OK):
		raise StopIteration

	    self.es.pc += 1

            if self._index < len(self._o._children):
                return self._o._children[self._index]
            else:
                raise StopIteration

    def __init__(self, file):
	sambase.ExecutionStateBase.__init__(self, file)

    def __iter__(self):
	return Iter(self)
