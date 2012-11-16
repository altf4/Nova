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
  borderDiv.setAttribute('style', 'border: 2px solid; background: #E8A02F; width: 450px;');
  
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

  /*var label5 = document.createElement('label');
  label5.innerHTML = 'Cron String: ';*/
  
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
  monthLabel.innerHTML = 'Month: ';
  mlTd.appendChild(monthLabel);
  var mTd = document.createElement('td');
  var month = document.createElement('input');
  month.setAttribute('style', 'width: 120px;');
  month.setAttribute('type', 'number');
  month.min = '0';
  month.max = '11';
  month.id = clientId + 'month';
  month.value = infoHook.getMonth();
  mTd.appendChild(month);
  tr0.appendChild(mlTd);
  tr0.appendChild(mTd);
  
  var tr1 = document.createElement('tr');
  var dlTd = document.createElement('td');
  var dayLabel = document.createElement('label');
  dayLabel.innerHTML = 'Day: ';
  dlTd.appendChild(dayLabel);
  var dTd = document.createElement('td');
  var day = document.createElement('select');
  day.setAttribute('style', 'width: 120px;');
  day.id = clientId + 'day';
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
  tr1.appendChild(dlTd);
  tr1.appendChild(dTd);
  
  var tr2 = document.createElement('tr');
  var ylTd = document.createElement('td');
  var yearLabel = document.createElement('label');
  yearLabel.innerHTML = 'Year: ';
  ylTd.appendChild(yearLabel);
  var yTd = document.createElement('td');
  var year = document.createElement('input');
  year.setAttribute('style', 'width: 120px;');
  year.setAttribute('type', 'number');
  year.min = infoHook.getFullYear();
  year.id = clientId + 'year';
  year.value = infoHook.getFullYear();
  yTd.appendChild(year);
  tr2.appendChild(ylTd);
  tr2.appendChild(yTd);
  
  var tr3 = document.createElement('tr');
  var hlTd = document.createElement('td');
  var hourLabel = document.createElement('label');
  hourLabel.innerHTML = 'Hour: ';
  hlTd.appendChild(hourLabel);
  var hTd = document.createElement('td');
  var hour = document.createElement('input');
  hour.setAttribute('style', 'width: 120px;');
  hour.id = clientId + 'hour';
  hour.setAttribute('type', 'number');
  hour.min = '0';
  hour.max = '23';
  hour.value = infoHook.getHours();
  hTd.appendChild(hour);
  tr3.appendChild(hlTd);
  tr3.appendChild(hTd);
  
  var tr4 = document.createElement('tr');
  var milTd = document.createElement('td');
  var minuteLabel = document.createElement('label');
  minuteLabel.innerHTML = 'Minute: ';
  milTd.appendChild(minuteLabel);
  var miTd = document.createElement('td');
  var minute = document.createElement('input');
  minute.setAttribute('style', 'width: 120px;');
  minute.setAttribute('type', 'number');
  minute.min = '0';
  minute.max = '59';
  minute.id = clientId + 'minute';
  minute.value = infoHook.getMinutes();
  miTd.appendChild(minute);
  tr4.appendChild(milTd);
  tr4.appendChild(miTd);
  
  var tr5 = document.createElement('tr');
  var slTd = document.createElement('td');
  var secondLabel = document.createElement('label');
  secondLabel.innerHTML = 'Second: ';
  slTd.appendChild(secondLabel);
  var sTd = document.createElement('td');
  var second = document.createElement('input');
  second.setAttribute('style', 'width: 120px;');
  second.setAttribute('type', 'number');
  second.min = '0';
  second.max = '59';
  second.id = clientId + 'second';
  second.value = infoHook.getSeconds();
  sTd.appendChild(second);
  tr5.appendChild(slTd);
  tr5.appendChild(sTd);

  var label4 = document.createElement('label');
  label4.innerHTML = 'Event Name: ';
  
  var name = document.createElement('input');
  name.id = clientId + 'name';
  name.maxlength = '10';
  
  var t = document.createElement('table');
  t.id = 'subjectToChange';
  
  var tbody = document.createElement('tbody');
  t.appendChild(tbody);
  
  tbody.appendChild(tr0);
  tbody.appendChild(tr1);
  tbody.appendChild(tr2);
  tbody.appendChild(tr3);
  tbody.appendChild(tr4);
  tbody.appendChild(tr5);
  
  var b0 = document.createElement('br');
  var b1 = document.createElement('br');
  
  borderDiv.appendChild(label0);
  borderDiv.appendChild(label1);
  borderDiv.appendChild(typeSelect);
  borderDiv.appendChild(recurring);
  borderDiv.appendChild(recurringLabel);
  borderDiv.appendChild(b0);
  borderDiv.appendChild(t);
  borderDiv.appendChild(b1);
  borderDiv.appendChild(label4);
  borderDiv.appendChild(name);
  
  document.getElementById('elementHook').appendChild(borderDiv);
}

function recurringChanged(source)
{
  // Restructure table here
  console.log('recurringChanged');
}

function newTypeSelected(clientId)
{
  // For when message types requiring arguments are supported
}

function registerScheduledMessage(clientId, name, message, date, cb)
{
  message.id = clientId + ':';
  now.SetScheduledMessage(clientId, name, message, date, cb);
}

function submitSchedule()
{
  for(var i in document.getElementById('elementHook').childNodes)
  {
    var id = document.getElementById('elementHook').childNodes[i].id; 
    if(id != undefined)
    {
      console.log('id ' + id);
      
      if(/^[a-z0-9]+$/i.test(document.getElementById(id + 'name').value) == false/* || document.getElementById(id + 'name').value.length <= 1*/)
      {
        document.getElementById(id + 'name').value = '';
        alert('Scheduled event names must consist of 2-10 alphanumeric characters');
        return;
      }
      if(document.getElementById(id + 'recurring').checked)
      {
        var recurrenceValues = {};
        console.log('Constructing recurrence rule parameters');
      }
      else
      {
        console.log('constructing Date for one time event');
        var year = (document.getElementById(id + 'year').value != '' ? document.getElementById(id + 'year').value : '0');
        var month = (document.getElementById(id + 'month').value != '' ? document.getElementById(id + 'month').value : '0');
        var day = (document.getElementById(id + 'day').value != '' ? document.getElementById(id + 'day').value : '0');
        var hour = (document.getElementById(id + 'hour').value != '' ? document.getElementById(id + 'hour').value : '0');
        var minute = (document.getElementById(id + 'minute').value != '' ? document.getElementById(id + 'minute').value : '0');
        var second = (document.getElementById(id + 'second').value != '' ? document.getElementById(id + 'second').value : '0');
        var submitDate = new Date(year, month, day, hour, minute, second);
        console.log('submitDate ' + submitDate.toDateString());
      }
    }
  }
}