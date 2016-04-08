// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//
//
// TODO documentation
//

import org.car2x.veins.nodes.Scenario;
import org.car2x.veins.nodes.RSUDdash;

network DDashNetwork extends Scenario
{
	types:
		channel gChannel extends ned.DelayChannel {
			delay = 10ms;
			}
    submodules:
        rsu[12]: RSUDdash {
            @display("p=150,140;b=10,10,oval");
        }
    connections:
    	rsu[0].nicGate++ <--> gChannel <--> rsu[1].nicGate++;
    	rsu[0].nicGate++ <--> gChannel <--> rsu[2].nicGate++;
    	rsu[0].nicGate++ <--> gChannel <--> rsu[3].nicGate++;
    	
    	rsu[1].nicGate++ <--> gChannel <--> rsu[3].nicGate++;
    	rsu[1].nicGate++ <--> gChannel <--> rsu[4].nicGate++;
    	
    	rsu[2].nicGate++ <--> gChannel <--> rsu[5].nicGate++;
    	rsu[2].nicGate++ <--> gChannel <--> rsu[7].nicGate++;
    	
    	rsu[3].nicGate++ <--> gChannel <--> rsu[5].nicGate++;
    	rsu[3].nicGate++ <--> gChannel <--> rsu[6].nicGate++;
    	rsu[3].nicGate++ <--> gChannel <--> rsu[8].nicGate++;
    	
    	rsu[4].nicGate++ <--> gChannel <--> rsu[6].nicGate++;
    	rsu[4].nicGate++ <--> gChannel <--> rsu[9].nicGate++;
    	
    	rsu[5].nicGate++ <--> gChannel <--> rsu[6].nicGate++;
    	rsu[5].nicGate++ <--> gChannel <--> rsu[7].nicGate++;
    	rsu[5].nicGate++ <--> gChannel <--> rsu[8].nicGate++;
    	
    	rsu[6].nicGate++ <--> gChannel <--> rsu[8].nicGate++;
    	rsu[6].nicGate++ <--> gChannel <--> rsu[9].nicGate++;
    	
    	rsu[7].nicGate++ <--> gChannel <--> rsu[10].nicGate++;
    	
    	rsu[8].nicGate++ <--> gChannel <--> rsu[10].nicGate++;
    	rsu[8].nicGate++ <--> gChannel <--> rsu[11].nicGate++;
    	
     	rsu[9].nicGate++ <--> gChannel <--> rsu[11].nicGate++;
 
    	rsu[10].nicGate++ <--> gChannel <--> rsu[11].nicGate++;
}

