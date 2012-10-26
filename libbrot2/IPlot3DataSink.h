/*
    Plot3Chunk.h: Data sink interface for Plot3Chunk.
    Copyright (C) 2012 Ross Younger

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef IPLOT3DATASINK_H_
#define IPLOT3DATASINK_H_

namespace Plot3 {

class IPlot3DataSink {
public:
	/**Signals that a chunk is complete, offering an opportunity to do
	 * something useful with the freshly-squeezed fractal data. The
	 * implementor is responsible for any sort of synchronisation they may
	 * require. */
	virtual void chunk_done(Plot3Chunk* job) = 0;

	/**Signals that a pass is completed.
	 * The string provides optional commentary about the plot so far.
	 * The implementor should not take too long here, as the next pass won't
	 * start until this function returns. */
	virtual void pass_complete(std::string&) = 0;

	/**Signals that the plot has finished work.
	 * It might have completed, or it might have been told to stop. */
	virtual void plot_complete() = 0;

	virtual ~IPlot3DataSink() {}
};

}

#endif /* IPLOT3DATASINK_H_ */
