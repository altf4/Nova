//============================================================================
// Name        : NovaPiChart.js
// Copyright   : DataSoft Corporation 2011-2013
//  Nova is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   Nova is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with Nova.  If not, see <http://www.gnu.org/licenses/>.
// Description : Simple pi chart
//============================================================================


// divId: id of div to put the pi chart
// size: Size of pi chart in pixels (will be size X size)
// items: Array of objects with each object consisting of,
//     name: String name of the item
//     value: Number of this item present
var NovaPiChart = function(divId, size, deleteButtonFunction) {
    this.m_id = divId;
    this.m_deleteFunction = deleteButtonFunction;
    this.SetSize(size);
}

NovaPiChart.prototype = {
    SetSize: function(size) {
        // Used so when we redrow we use the same colors
        this.m_usedColors = new Object();
        this.m_size = size;
        this.m_halfSize = parseInt(size/2);
    },

    // Renders the chart
    Render: function(items) {
        if (items !== undefined) {
            this.m_items = items;
        }

        this.m_numberOfItems = 0;
        for (var i = 0; i < items.length; i++) {
           this.m_numberOfItems += items[i].value;
        }
        // Reset the div
        document.getElementById(this.m_id).innerHTML = "";

        // Make a canvas
        var canvas = document.createElement("canvas");
        canvas.setAttribute("width", this.m_size + "px");
        canvas.setAttribute("height", this.m_size + "px");
        document.getElementById(this.m_id).appendChild(canvas);
        
        // Draw the pi chart on the canvas
        var ctx = canvas.getContext("2d");
        var lastend = 0;
        var randomColor;
        ctx.clearRect(0, 0, canvas.width, canvas.height);

        if (this.m_numberOfItems == 0) {
            ctx.fillStyle = "#A1A1A1";
            ctx.beginPath();
            ctx.moveTo(this.m_halfSize,this.m_halfSize);
            ctx.arc(this.m_halfSize,this.m_halfSize, this.m_halfSize,lastend,lastend+Math.PI*2,false);
            ctx.lineTo(this.m_halfSize, this.m_halfSize);
            ctx.fill();

            for (var i = 0; i < this.m_items.length; i++) {
                var legend = document.createElement("div");
                var text = document.createElement("p");
                text.setAttribute('style', 'display: inline-block; margin: 0px');
                text.innerHTML = "<span style='background-color: " + "#A1A1A1" + ";'>&nbsp &nbsp &nbsp</span>&nbsp 0% " + this.m_items[i].name;
                legend.appendChild(text);
                document.getElementById(this.m_id).appendChild(legend);
            }
            return;
        }
        for (var pfile = 0; pfile < this.m_items.length; pfile++) {
            if (this.m_usedColors[this.m_items[pfile].name] === undefined) {
                randomColor = ((1<<24)*Math.random()|0).toString(16);
                // Pad color with 0's on the left
                if (randomColor.length != 6) {
                    for (var i = randomColor.length; i < 6; i++) {
                        randomColor = "0" + randomColor;
                    }
                }
                randomColor = "#" + randomColor;
                this.m_usedColors[this.m_items[pfile].name] = randomColor;
            } else {
                randomColor = this.m_usedColors[this.m_items[pfile].name];
            }

            ctx.fillStyle = randomColor;
            ctx.beginPath();
            ctx.moveTo(this.m_halfSize,this.m_halfSize);
            ctx.arc(this.m_halfSize,this.m_halfSize, this.m_halfSize,lastend,lastend+(Math.PI*2*(this.m_items[pfile].value/this.m_numberOfItems)),false);
            ctx.lineTo(this.m_halfSize, this.m_halfSize);
            ctx.fill();
            lastend += Math.PI*2*(this.m_items[pfile].value/this.m_numberOfItems);

            // Draw the legend and values
            var legend = document.createElement("div");
            var text = document.createElement("p");
            text.innerHTML = "<span style='background-color: " + randomColor + ";'>&nbsp &nbsp &nbsp</span>&nbsp " +  (100*this.m_items[pfile].value/this.m_numberOfItems).toFixed(2) + "% (" + this.m_items[pfile].value + ") " + this.m_items[pfile].name;
            text.setAttribute('style', 'display: inline-block; margin: 0px');
            if(this.m_deleteFunction != undefined) {
                var deleteButton = document.createElement("button");
                deleteButton.setAttribute('style', 'display: inline-block;');
                deleteButton.innerHTML = "Delete All";
                deleteButton.setAttribute('onclick', this.m_deleteFunction + '("' + this.m_items[pfile].name + '")');
                legend.appendChild(deleteButton);
            }
            legend.appendChild(text);
            document.getElementById(this.m_id).appendChild(legend);
        }
        
    }
}
