function watermark(id, text)
{
  var element = document.getElementById(id);

  if(element.value.length > 0)
  {
    if(element.value == text)
    {
      element.value = '';
      element.setAttribute('style', 'width: 120px;');
    }
  }
  else
  {
    element.value = text;
    element.setAttribute('style', 'font-style: italic; width: 120px;');
  }
}

function setTarget(source, target, group)
{
  if(group == 'true' || group == true)
  {
    while(document.getElementById('elementHook').hasChildNodes())
    {
      document.getElementById('elementHook').removeChild(document.getElementById('elementHook').lastChild);
    }
    
    if(document.getElementById(source).checked)
    {
      message.id = target + ':';
      var targets = target.split(':');

      for(var i in targets)
      {
        if(targets[i] != '' && targets[i] != undefined)
        {
          createScheduledEventElement(targets[i]);
        } 
      }

      for(var i in document.getElementById('groupsList').childNodes)
      {
        if(document.getElementById('groupcheck' + i) != undefined && document.getElementById('groupcheck' + i).id.indexOf(source) == -1)
        {
          document.getElementById('groupcheck' + i).setAttribute('disabled', true);
        }
      }
      for(var i in document.getElementById('clientsList').childNodes)
      {
        if(document.getElementById('check' + i) != undefined)
        {
          document.getElementById('check' + i).checked = false;
          document.getElementById('check' + i).setAttribute('disabled', true);
        } 
      }
    }
    else
    {
      message.id = '';
      var targets = target.split(':');
      
      while(document.getElementById('elementHook').hasChildNodes())
      {
        document.getElementById('elementHook').removeChild(document.getElementById('elementHook').lastChild);
      }
      for(var i in document.getElementById('groupsList').childNodes)
      {
        if(document.getElementById('groupcheck' + i) != undefined && document.getElementById('groupcheck' + i).id.indexOf(source) == -1)
        {
          document.getElementById('groupcheck' + i).removeAttribute('disabled');
        }
      }
      for(var i in document.getElementById('clientsList').childNodes)
      {
        if(document.getElementById('check' + i) != undefined)
        {
          document.getElementById('check' + i).removeAttribute('disabled');
        } 
      }
    }
  }
  else
  {
    if(document.getElementById(source).checked)
    {
      message.id += target + ':';
      createScheduledEventElement(target);
    }
    else
    {
      var regex = new RegExp(target + ':', 'i');
      message.id = message.id.replace(regex, '');
      document.getElementById('elementHook').removeChild(document.getElementById(target));
    }
  }
}

function createScheduledEventElement(clientId)
{
  var borderDiv = document.createElement('div');
  borderDiv.id = clientId;
  borderDiv.setAttribute('style', 'border: 2px solid; background: #E8A02F; width: 370px;');
  
  var label0 = document.createElement('h1');
  label0.setAttribute('style', 'text-align: center');
  label0.innerHTML = clientId;
  
  var label1 = document.createElement('label');
  label1.innerHTML = 'Message Type: ';
  
  var typeSelect = document.createElement('select');
  typeSelect.id = clientId + 'select';
  
  for(var i in messageTypes)
  {
    if(messageTypes[i] != '' && messageTypes[i] != undefined)
    {
      var option = document.createElement('option');
      option.value = messageTypes[i];
      option.innerHTML = messageTypes[i];
      typeSelect.appendChild(option);
    }
  }
  
  typeSelect.setAttribute('onclick', 'newTypeSelected("' + clientId + '")');
  
  var recurring = document.createElement('input');
  recurring.setAttribute('type', 'checkbox');
  recurring.id = clientId + 'recurring';
  recurring.checked = false;
  recurring.setAttribute('onclick', 'recurringChanged("' + clientId + '")');
  
  var recurringLabel = document.createElement('label');
  recurringLabel.innerHTML = 'Recurring?';
  
  var infoHook = new Date();
  
  var tr0 = document.createElement('tr');
  var mlTd = document.createElement('td');
  var monthLabel = document.createElement('label');
  monthLabel.innerHTML = 'M ';
  mlTd.appendChild(monthLabel);
  var month = document.createElement('input');
  month.setAttribute('style', 'width: 30px;');
  month.setAttribute('type', 'number');
  month.min = '0';
  month.max = '11';
  month.id = clientId + 'month';
  month.value = infoHook.getMonth();
  mlTd.appendChild(month);
  tr0.appendChild(mlTd);
  
  var dlTd = document.createElement('td');
  var dayLabel = document.createElement('label');
  dayLabel.innerHTML = '/D ';
  dlTd.appendChild(dayLabel);
  var day = document.createElement('input');
  day.setAttribute('type', 'number');
  day.setAttribute('style', 'width: 30px;');
  day.min = '1';
  day.max = '31';
  day.value = infoHook.getDate();
  day.id = clientId + 'day';
  dlTd.appendChild(day);
  tr0.appendChild(dlTd);
  
  var ylTd = document.createElement('td');
  var yearLabel = document.createElement('label');
  yearLabel.innerHTML = '/Y ';
  ylTd.appendChild(yearLabel);
  var year = document.createElement('input');
  year.setAttribute('style', 'width: 45px;');
  year.setAttribute('type', 'number');
  year.min = infoHook.getFullYear();
  year.id = clientId + 'year';
  year.value = infoHook.getFullYear();
  ylTd.appendChild(year);
  tr0.appendChild(ylTd);
  
  var tr1 = document.createElement('tr');
  var hlTd = document.createElement('td');
  var hourLabel = document.createElement('label');
  hourLabel.innerHTML = 'H ';
  hlTd.appendChild(hourLabel);
  var hour = document.createElement('input');
  hour.setAttribute('style', 'width: 30px;');
  hour.id = clientId + 'hour';
  hour.setAttribute('type', 'number');
  hour.min = '0';
  hour.max = '23';
  hour.value = infoHook.getHours();
  hlTd.appendChild(hour);
  tr1.appendChild(hlTd);
  
  var milTd = document.createElement('td');
  var minuteLabel = document.createElement('label');
  minuteLabel.innerHTML = ':M ';
  milTd.appendChild(minuteLabel);
  var minute = document.createElement('input');
  minute.setAttribute('style', 'width: 30px;');
  minute.setAttribute('type', 'number');
  minute.min = '0';
  minute.max = '59';
  minute.id = clientId + 'minute';
  minute.value = infoHook.getMinutes();
  milTd.appendChild(minute);
  tr1.appendChild(milTd);

  var label4 = document.createElement('label');
  label4.innerHTML = 'Event Name: ';
  
  var name = document.createElement('input');
  name.id = clientId + 'name';
  name.maxlength = '10';
  
  var t = document.createElement('table');
  t.id = clientId + 'subjectToChange';
  
  var tbody = document.createElement('tbody');
  tbody.id = clientId + 'changeHook';
  t.appendChild(tbody);
  
  tbody.appendChild(tr0);
  tbody.appendChild(tr1);
  
  var b0 = document.createElement('br');
  var b1 = document.createElement('br');
  
  borderDiv.appendChild(label0);
  borderDiv.appendChild(label1);
  borderDiv.appendChild(typeSelect);
  borderDiv.appendChild(recurring);
  borderDiv.appendChild(recurringLabel);
  borderDiv.appendChild(b0);
  borderDiv.appendChild(t);
  borderDiv.appendChild(label4);
  borderDiv.appendChild(name);
  borderDiv.appendChild(b1);
  
  document.getElementById('elementHook').appendChild(borderDiv);
}

function recurringChanged(source)
{
  while(document.getElementById(source + 'changeHook').hasChildNodes())
  {
    document.getElementById(source + 'changeHook').removeChild(document.getElementById(source + 'changeHook').lastChild);
  }
  
  var infoHook = new Date();
  
  if(document.getElementById(source + 'recurring').checked)
  {
    var tr0 = document.createElement('tr');
    var dlTd = document.createElement('td');
    var dayLabel = document.createElement('label');
    dayLabel.innerHTML = 'Day Of Week: ';
    dlTd.appendChild(dayLabel);
    var dTd = document.createElement('td');
    var day = document.createElement('select');
    day.setAttribute('style', 'width: 100px;');
    day.id = source + 'dayOfWeek';
    var options = ['Sunday', 'Monday', 'Tuesday', 'Wednesday', 'Thursday', 'Friday', 'Saturday'];
    for(var i in options)
    {
      var option = document.createElement('option');
      option.id = options[i];
      option.innerHTML = options[i];
      option.value = infoHook.getDay().toString();
      switch(infoHook.getDay().toString())
      {
        case '0': selectedTest = 'Sunday';
                  if(selectedTest == options[i])
                  {
                    option.selected = true;
                  }
                  break;
        case '1': selectedTest = 'Monday';
                  if(selectedTest == options[i])
                  {
                    option.selected = true;
                  }
                  break;
        case '2': selectedTest = 'Tuesday';
                  if(selectedTest == options[i])
                  {
                    option.selected = true;
                  }
                  break;
        case '3': selectedTest = 'Wednesday';
                  if(selectedTest == options[i])
                  {
                    option.selected = true;
                  }
                  break;
        case '4': selectedTest = 'Thursday';
                  if(selectedTest == options[i])
                  {
                    option.selected = true;
                  }
                  break;
        case '5': selectedTest = 'Friday';
                  if(selectedTest == options[i])
                  {
                    option.selected = true;
                  }
                  break;
        case '6': selectedTest = 'Saturday';
                  if(selectedTest == options[i])
                  {
                    option.selected = true;
                  }
                  break;
        default:  console.log('infoHook.getDay() ' + infoHook.getDay());
                  break;
      }
      day.appendChild(option);
    }
    dTd.appendChild(day);
    tr0.appendChild(dlTd);
    tr0.appendChild(dTd);
    
    var tr1 = document.createElement('tr');
    var hlTd = document.createElement('td');
    var hourLabel = document.createElement('label');
    hourLabel.innerHTML = 'H ';
    hlTd.appendChild(hourLabel);
    var hour = document.createElement('input');
    hour.setAttribute('style', 'width: 30px;');
    hour.id = source + 'hour';
    hour.setAttribute('type', 'number');
    hour.min = '0';
    hour.max = '23';
    hour.value = infoHook.getHours();
    hlTd.appendChild(hour);
    tr1.appendChild(hlTd);
    
    var milTd = document.createElement('td');
    var minuteLabel = document.createElement('label');
    minuteLabel.innerHTML = ':M ';
    milTd.appendChild(minuteLabel);
    var minute = document.createElement('input');
    minute.setAttribute('style', 'width: 30px;');
    minute.setAttribute('type', 'number');
    minute.min = '0';
    minute.max = '59';
    minute.id = source + 'minute';
    minute.value = infoHook.getMinutes();
    milTd.appendChild(minute);
    tr1.appendChild(milTd);
    
    document.getElementById(source + 'changeHook').appendChild(tr0);
    document.getElementById(source + 'changeHook').appendChild(tr1);
  }
  else
  {
    var tr0 = document.createElement('tr');
    var mlTd = document.createElement('td');
    var monthLabel = document.createElement('label');
    monthLabel.innerHTML = 'M ';
    mlTd.appendChild(monthLabel);
    var month = document.createElement('input');
    month.setAttribute('style', 'width: 30px;');
    month.setAttribute('type', 'number');
    month.min = '0';
    month.max = '11';
    month.id = source + 'month';
    month.value = infoHook.getMonth();
    mlTd.appendChild(month);
    tr0.appendChild(mlTd);
    
    var dlTd = document.createElement('td');
    var dayLabel = document.createElement('label');
    dayLabel.innerHTML = '/D ';
    dlTd.appendChild(dayLabel);
    var dTd = document.createElement('td');
    var day = document.createElement('input');
    day.setAttribute('type', 'number');
    day.setAttribute('style', 'width: 30px;');
    day.min = '1';
    day.max = '31';
    day.id = source + 'day';
    day.value = infoHook.getDate();
    dlTd.appendChild(day);
    tr0.appendChild(dlTd);
    
    var ylTd = document.createElement('td');
    var yearLabel = document.createElement('label');
    yearLabel.innerHTML = '/Y ';
    ylTd.appendChild(yearLabel);
    var year = document.createElement('input');
    year.setAttribute('style', 'width: 45px;');
    year.setAttribute('type', 'number');
    year.min = infoHook.getFullYear();
    year.id = source + 'year';
    year.value = infoHook.getFullYear();
    ylTd.appendChild(year);
    tr0.appendChild(ylTd);
    
    var tr1 = document.createElement('tr');
    var hlTd = document.createElement('td');
    var hourLabel = document.createElement('label');
    hourLabel.innerHTML = 'H ';
    hlTd.appendChild(hourLabel);
    var hour = document.createElement('input');
    hour.setAttribute('style', 'width: 30px;');
    hour.id = source + 'hour';
    hour.setAttribute('type', 'number');
    hour.min = '0';
    hour.max = '23';
    hour.value = infoHook.getHours();
    hlTd.appendChild(hour);
    tr1.appendChild(hlTd);
    
    var milTd = document.createElement('td');
    var minuteLabel = document.createElement('label');
    minuteLabel.innerHTML = ':M ';
    milTd.appendChild(minuteLabel);
    var minute = document.createElement('input');
    minute.setAttribute('style', 'width: 30px;');
    minute.setAttribute('type', 'number');
    minute.min = '0';
    minute.max = '59';
    minute.id = source + 'minute';
    minute.value = infoHook.getMinutes();
    milTd.appendChild(minute);
    tr1.appendChild(milTd);
    
    document.getElementById(source + 'changeHook').appendChild(tr0);
    document.getElementById(source + 'changeHook').appendChild(tr1);
  }
}

function newTypeSelected(clientId)
{
  // For when message types requiring arguments are supported
}

function registerScheduledMessage(clientId, name, message, eventObj, cb)
{
  message.id = clientId + ':';
  now.SetScheduledMessage(clientId, name, message, eventObj, cb);
}

function submitSchedule()
{
  for(var i in document.getElementById('elementHook').childNodes)
  {
    var id = document.getElementById('elementHook').childNodes[i].id; 
    if(id != undefined)
    {
      if(/^[a-z0-9]+$/i.test(document.getElementById(id + 'name').value) == false/* || document.getElementById(id + 'name').value.length <= 1*/)
      {
        document.getElementById(id + 'name').value = '';
        alert('Scheduled event names must consist of 2-10 alphanumeric characters');
        return;
      }
      var name = document.getElementById(id + 'name').value;
      if(document.getElementById(id + 'recurring').checked)
      {
        var recurrenceValues = {};
        var dayOfWeek = (document.getElementById(id + 'dayOfWeek').value != '' ? document.getElementById(id + 'dayOfWeek').value : '0');
        var hour = (document.getElementById(id + 'hour').value != '' ? document.getElementById(id + 'hour').value : '0');
        var minute = (document.getElementById(id + 'minute').value != '' ? document.getElementById(id + 'minute').value : '0');
        recurrenceValues.dayOfWeek = dayOfWeek;
        recurrenceValues.hour = hour;
        recurrenceValues.minute = minute;
        message.type = document.getElementById(id + 'select').value;
        registerScheduledMessage(id, name, message, recurrenceValues, function(clientId, result){
          console.log('Event registration for ' + clientId + ' ' + result);
        });
      }
      else
      {
        var year = (document.getElementById(id + 'year').value != '' ? document.getElementById(id + 'year').value : '0');
        var month = (document.getElementById(id + 'month').value != '' ? document.getElementById(id + 'month').value : '0');
        var day = (document.getElementById(id + 'day').value != '' ? document.getElementById(id + 'day').value : '0');
        var hour = (document.getElementById(id + 'hour').value != '' ? document.getElementById(id + 'hour').value : '0');
        var minute = (document.getElementById(id + 'minute').value != '' ? document.getElementById(id + 'minute').value : '0');
        var submitDate = new Date(year, month, day, hour, minute);
        message.type = document.getElementById(id + 'select').value;
        registerScheduledMessage(id, name, message, submitDate, function(clientId, result){
          console.log('Event registration for ' + clientId + ' ' + result);
        });
      }
      
      var j = 0;
      var element = document.getElementById('check' + j).value;
      
      do
      {
        if(element == id)
        {
          document.getElementById('check' + j).checked = false;
          element = undefined;
        } 
      }while(element != undefined && j++);
    }
  }
  while(document.getElementById('elementHook').hasChildNodes())
  {
    document.getElementById('elementHook').removeChild(document.getElementById('elementHook').lastChild);
  }
}