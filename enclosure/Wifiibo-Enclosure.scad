$fn=180;

// NFC Board Type
//NFC_Type = "PN532"; // [PN532,MFRC522]

// NFC Board Type
NFC_Type = "MFRC522"; // [PN532,MFRC522]

// NFC Board Mounting
NFC_Mount_Type = "PressureFit"; // [PressureFit,Standoffs]

// NFC Board Mounting
//NFC_Mount_Type = "Standoffs"; // [PressureFit,Standoffs]

cylInnerDia = (NFC_Type=="PN532") ? 58 : 78; // 58 for PN532, 90 for MFRC522

cylThickness = 3;
cylOuterDia = cylInnerDia + cylThickness;
cylHeight = 15;
cylBaseDia = cylInnerDia + 15;
cylBaseHeight = 12; // Originally 15; 12 might work?
topLipHeight = 2;
topLipDia = 54;

standoffR1 = 7.63/2;
standoffR2 = 5.5/2;
standoffHole = 2.2/2;

wemosWidth = 25.75;
wemosLength = 34.25;
wemosThickness = 1.05;

pn532HoleDist = 37.3;
pn532BoardWidth = 43;
pn532BoardLength = 40.75;

mfrc522BoardWidth = 39;
mfrc522BoardLength = 59.75;
mfrc522HoleY1Dist = 25;
mfrc522HoleY2Dist = 34;
mfrc522HoleOffsets = 37.5;

amiiboBaseDia = 49.5;

//translate([0, 0, 15])
//tagLid();
//translate([0, 0, -cylBaseHeight/2-cylHeight/2-5])
tagBase();

module standoff(height)
{
    difference()
    {
        cylinder(r1=standoffR1, r2=standoffR2, h=height, center=true);
        cylinder(r=standoffHole, h=height+0.01, center=true);
    }
}

module tagLid()
{
    // Main Body
    difference()
    {
        cylinder(r1=cylBaseDia/2, r2=cylOuterDia/2, h=cylHeight, center=true);
        //translate([0, 0, -cylThickness])
        cylinder(r1=cylBaseDia/2-cylThickness, r2=cylOuterDia/2-cylThickness, h=cylHeight+0.01, center=true);
    }
    
    // Top
    topRoundRad = 2;
    translate([0, 0, cylHeight/2-topLipHeight-0.5])
    difference()
    {
        cylinder(r=cylOuterDia/2, h=cylThickness+topLipHeight, center=true);
        translate([0, 0, cylThickness/2+topRoundRad-0.5])
        minkowski()
        {
            cylinder(r=topLipDia/2-topRoundRad, h=topLipHeight+0.01-topRoundRad, center=true);
            sphere(r=topRoundRad, $fn=25);
        }
    }
    
    // Connecting 'lip'
    lipWidth=2;
    lipHeight=5;
    lipHeightExtra=4;
    translate([0, 0, -cylHeight/2+lipHeightExtra/2])
    difference()
    {
        cylinder(r=cylBaseDia/2-cylThickness, h=lipHeight+lipHeightExtra, center=true);
        cylinder(r=cylBaseDia/2-cylThickness-lipWidth, h=lipHeight+lipHeightExtra+0.01, center=true);
        translate([1, -2, -5])
        microUSBHole();
    }
    
    if (NFC_Type=="PN532")
    {
        if (NFC_Mount_Type=="PressureFit")
        {
            tagLidPressureFit(cylInnerDia-13.7, pn532BoardWidth);
        }
        else
        {
            tagLidPN532Standoffs();
        }
    }
    else
    {
        if (NFC_Mount_Type=="PressureFit")
        {
            tagLidPressureFit(cylInnerDia-5, mfrc522BoardWidth);
        }
        else
        {
            tagLidMFRC522Standoffs();
        }
    }
}

module tagLidMFRC522Standoffs()
{
    standoffHeight = 6;
    translate([0, mfrc522BoardLength/2-3, 0])
    rotate([180, 0, 0])
    union()
    {
        translate([mfrc522HoleY1Dist/2, 0, 0])
        standoff(standoffHeight);
        translate([-mfrc522HoleY1Dist/2, 0, 0])
        standoff(standoffHeight);
        
        translate([0, mfrc522HoleOffsets, 0])
        union()
        {
            translate([mfrc522HoleY2Dist/2, 0, 0])
            standoff(standoffHeight);
            translate([-mfrc522HoleY2Dist/2, 0, 0])
            standoff(standoffHeight);
        }
    }
}

module tagLidPN532Standoffs()
{
    standoffHeight = 6;
    translate([3, 0, 0])
    rotate([180, 0, 45])
    union()
    {
        translate([pn532HoleDist/2, 0, 0])
        standoff(standoffHeight);
        translate([-pn532HoleDist/2, 0, 0])
        standoff(standoffHeight);
    }
}

module tagLidPressureFit(railLen, width)
{
    mountH = 5;
    mountW = 2;

    translate([0, railLen/2, 3-mountH])
    rotate([0, 0, 180])
    union()
    {
        translate([width/2, 0, 0])
        cube([mountW, railLen, mountH]);
        translate([-width/2-mountW, 0, 0])
        cube([mountW, railLen, mountH]);
        //translate([-pn532BoardWidth/2, 0, 0])
        //cube([pn532BoardWidth, mountW, mountH]); 
    }
}

module tagBase()
{
    wemosYOffset = -1;
    difference()
    {
        cylinder(r=cylBaseDia/2, h=cylBaseHeight, center=true);
        translate([0, 0, cylThickness])
        cylinder(r=cylBaseDia/2-cylThickness, h=cylBaseHeight, center=true);
        
        //translate([0, wemosLength/2+wemosYOffset, -2.5])
        //#wemosBoard();
        
        // Hole for MicroUSB
        translate([1, -10, 0])
        microUSBHole();
    }
    
    wemosMountH = 4;
    wemosMountW = 2;        
    translate([0, cylBaseDia/2-wemosLength-8.25, -cylBaseHeight/2+cylThickness])
    union()
    {
        translate([wemosWidth/2, 0, 0])
        cube([wemosMountW, wemosLength-5, wemosMountH]);
        translate([-wemosWidth/2-wemosMountW, 0, 0])
        cube([wemosMountW, wemosLength-1, wemosMountH]);
        translate([-wemosWidth/2, 0, 0])
        cube([wemosWidth, wemosMountW, wemosMountH]); 
    }
}

module microUSBHole()
{
    microUSB_W = 13;
    microUSB_H = 8;
    
    translate([-microUSB_W/2, cylBaseDia/2-cylThickness-2, -cylBaseHeight/2+cylThickness-1.5])
    cube([microUSB_W, 20, microUSB_H]);
}

module wemosBoard()
{
    color("blue")
    difference()
    {
        cube([wemosWidth, wemosLength, wemosThickness], center=true);
    }
}