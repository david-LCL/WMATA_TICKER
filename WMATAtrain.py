import serial, time, sys
from pywmata import Wmata
 
#SERIALPORT = "5" # this is my USB serial port YMMV
#remember there is a max query from web per time
API_KEY = '' #your API KEY goes here
RAIL_STOP_ID = ''#your stop goes here
def fetchTrain():
        api = Wmata('API_KEY')
        pentagonList = api.rail_predictions('RAIL_STOP_ID')
        print"got from web"
        return pentagonList
#        while pentagonList:
#                tData= pentagonList.pop()
#                print"parsing"
#                dTime = tData["Min"]
#                dName = tData["DestinationName"]
#                stat= dTime+" "+dName

#                print"to lcd"
#                pageText(stat,s)
#                time.sleep(30)
def fetchBus():
        print "running"
        api = Wmata('API_KEY')
        busList = api.bus_schedule_by_stop('6000263') #use nextbus to find stop id
        print "got bus list \n"
        i = 0
        return busList

def pageText(textstring, s):
    botline = ""
    cursor = 0
    for letter in textstring:
        # print letter, cursor   # this is for debugging
        s.write(letter)

        if cursor > 15:
            # I'm printing in second line so keep track of what I write
            botline = botline + letter
            # print botline

        if cursor == 31: # page the bottom line up to top, clear bottom, and write
            # print "cursor wrap"
            s.write('\xFE\x80') # wrap to start of first line
            s.write(botline) # write what was on the bottom (now on top)
            s.write("                ")
            s.write('\xFE\xC0') # skip to beginning of second line
            botline = ""
            cursor = 15 # reset to beginning of second line

        cursor = cursor + 1

        time.sleep(0.5) # set this delay to a comfortable value

def scrollText(textstring, s):
    nextstring = ""
    cursor = 0
    firstpass = True # test whether this is the first 16 characters
    for letter in textstring:
        if firstpass == False:
            s.write('\xFE\x18') # scroll left one spot at each letter

        # print letter, cursor  # this is for debugging
        s.write(letter)

        if cursor == 15:
            # I'm printing the last visible character
            s.write('\xFE\x90') # jump cursor to 2nd column of 16
            firstpass = False # once the first row is filled, we need to scroll

        if cursor == 31:
            s.write('\xFE\xA0') # jump cursor to 3rd column

        if cursor == 39:
            cursor = -1 # start over, there are only 40 characters in memory
            s.write('\xFE\x80') # this is the original character address.

        cursor = cursor + 1

        time.sleep(3) # adjust this to a comfortable value
def main():
    s = serial.Serial(4, 9600)
    s.close()
    s.open()
    time.sleep(2)
    s.write('\xFE\x01') # clear the LCD screen
    while(True):
#        time.sleep(30)
        s.write('\xFE\x80') # goto 0 position
        trainList=fetchTrain()
        while trainList:
                tData= trainList.pop()
                print"parsing"
                dTime = tData["Min"]
                dName = tData["DestinationName"]
                stat= dTime+" "+dName
                pageText(stat,s)
                time.sleep(5)
                s.write('\xFE\x01')
        s.write('\xFE\x01') # clear the screen (in preparation to repeat)
        s.write('\xFE\x80') # goto 0 position
        bList=fetchBus()
        while bList["ScheduleArrivals"]:
            bData=bList["ScheduleArrivals"].pop()
            bname=bData["RouteID"]
            bTime=bData["ScheduleTime"]
            ima = time.localtime()
            busTimeG=time.strptime(bTime, "%Y-%m-%dT%H:%M:%S") # parses bTime correctly              
            if(busTimeG.tm_hour == ima.tm_hour):
                #print "more bus are coming"
                if(busTimeG.tm_min>= ima.tm_min):
                    stat = bname+" "+str(busTimeG.tm_hour)+":"+str(busTimeG.tm_min)+"\n"
                    pageText(stat,s)
                    time.sleep(5)
                    s.write('\xFE\x01')
            if(busTimeG.tm_hour>ima.tm_hour):
                #print "bus within 2 hours \n"
                if(busTimeG.tm_hour<=(ima.tm_hour+1)):
                    if(busTimeG.tm_min<=10):
                        stat= bname+" "+str(busTimeG.tm_hour)+":"+str(busTimeG.tm_min)+"\n"
                        pageText(stat,s)
                        time.sleep(5)
                        s.write('\xFE\x01')
        

    s.close()

if __name__ == '__main__':
    main()
