$fn=180;

// NFC Board Type
//NFC_Type = "PN532"; // [PN532,MFRC522]

// NFC Board Type
NFC_Type = "MFRC522"; // [PN532,MFRC522]

// NFC Board Mounting
NFC_Mount_Type = "PressureFit"; // [PressureFit,Standoffs]

// NFC Board Mounting
//NFC_Mount_Type = "Standoffs"; // [PressureFit,Standoffs]

pn532BoardWidth = 43;
pn532BoardLength = 40.75;
pn532HoleDist = 37.3;
pn532HoleOffset = 3;
pn532BoardZOffset = 0;

mfrc522BoardWidth = 36.35;
mfrc522BoardLength = 36.35;
mfrc522HoleDist = 30.5;
mfrc522HoleOffset = 0;
mfrc522BoardZOffset = 2;

nfcBoardWidth = (NFC_Type=="PN532") ? pn532BoardWidth : mfrc522BoardWidth;
nfcBoardLength = (NFC_Type=="PN532") ? pn532BoardLength : mfrc522BoardLength;
nfcBoardHoleDist = (NFC_Type=="PN532") ? pn532HoleDist : mfrc522HoleDist;
nfcBoardHoleOffset = (NFC_Type=="PN532") ? pn532HoleOffset : mfrc522HoleOffset;
nfcBoardZOffset = (NFC_Type=="PN532") ? pn532BoardZOffset : mfrc522BoardZOffset;

//cylInnerDia = (NFC_Type=="PN532") ? 58 : 58; // 58 for PN532, 90 for MFRC522
cylInnerDia = 58;

cylThickness = 3;
cylOuterDia = cylInnerDia + cylThickness;
cylHeight = 15 + nfcBoardZOffset;
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

resetButtonRad = 2;
resetButtonLen = 16;
resetButtonX = wemosWidth/2-1;
resetButtonY = wemosLength-7;

amiiboBaseDia = 49.5;
femalePinHeaderHeight = 8.4;
pinHeaderWidth = 2.5;

//translate([0, 0, 15])
tagLid();
//translate([0, 0, -cylBaseHeight/2-cylHeight/2-5])
//tagBase(true);
//wemosBoard();

//rotate([0, -90, 0])
//translate([-resetButtonX, -resetButtonY, 0])
//resetButton();
//resetButtonMountTest();
//buttonTest();
//import("Wifiibo-Enclosure-PN532-Top-PF.stl");

module buttonTest()
{
    difference()
    {
        translate([0, 0, cylBaseHeight/2+0.01])
        tagBase();
        
        // Right cutoff
        translate([wemosWidth/2+14, -cylBaseDia/2, 0])
        cube([cylBaseDia, cylBaseDia, cylHeight]);
        // Left cutoff
        translate([-cylBaseDia-wemosWidth/2-3, -cylBaseDia/2, 0])
        cube([cylBaseDia, cylBaseDia, cylHeight]);
        // Back cutoff
        translate([-cylBaseDia/2, -cylBaseDia-7, 0])
        cube([cylBaseDia, cylBaseDia, cylHeight]);
    }
}

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
    translate([0, 0, cylHeight/2-topLipHeight-0.5-nfcBoardZOffset/2])
    difference()
    {
        cylinder(r=cylOuterDia/2, h=cylThickness+topLipHeight+nfcBoardZOffset, center=true);
        translate([0, 0, cylThickness/2+topRoundRad-0.5+nfcBoardZOffset/2])
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
    
    c = cylOuterDia/2;
    a = nfcBoardWidth/2;
    railLen = sqrt(c*c-a*a)*2 + 1.3;
    echo(railLen);
    echo(cylInnerDia-13.7);

    if (NFC_Mount_Type=="PressureFit")
    {
        tagLidPressureFit(railLen, nfcBoardWidth);
    }
    else
    {
        tagLidStandoffs();
    }
}

module tagLidStandoffs()
{
    standoffHeight = 6;
    translate([nfcBoardHoleOffset, 0, 0])
    rotate([180, 0, 45])
    union()
    {
        translate([nfcBoardHoleDist/2, 0, 0])
        standoff(standoffHeight);
        translate([-nfcBoardHoleDist/2, 0, 0])
        standoff(standoffHeight);
    }
}

module tagLidPressureFit(railLen, width)
{
    mountH = 5;
    mountW = 2;

    translate([0, railLen/2, cylHeight/2-7.5-nfcBoardZOffset+0.01])
    rotate([0, 0, 180])
    union()
    {
        translate([width/2+mountW/2, railLen/2, 0])
        cube([mountW, railLen, mountH], center=true);
        translate([-width/2-mountW/2, railLen/2, 0])
        cube([mountW, railLen, mountH], center=true);
        //translate([-pn532BoardWidth/2, 0, 0])
        //cube([pn532BoardWidth, mountW, mountH]); 
    }
}

module resetButtonMountTest()
{
    difference()
    {
            cubeExtraY = 3;
            cubeExtraZ = 5;
            //translate([resetButtonX+0.01, resetButtonY-resetButtonRad-cubeExtraY/6-0.51, -resetButtonRad-cubeExtraZ/6-1])
            {
                buttonCubeX = resetButtonLen-9.1;
                buttonCubeY = resetButtonRad+cubeExtraY+2;
                buttonCubeZ = resetButtonRad+cubeExtraY+2;
                cube([buttonCubeX, buttonCubeY, buttonCubeZ]);
                translate([1-0.01, -2, 0])
                cube([buttonCubeX-1+0.01, 2, buttonCubeZ]);
            }
        
        translate([-1, 0, 0])
        scale([1.05, 1, 1])
        resetButton();
    }
}

module tagBase(resetHole=false)
{
    wemosYOffset = -1;
    
    difference()
    {
        union()
        {
            difference()
            {
                cylinder(r=cylBaseDia/2, h=cylBaseHeight, center=true);        
                translate([0, 0, cylThickness])
                cylinder(r=cylBaseDia/2-cylThickness, h=cylBaseHeight, center=true);
            }
            
            //cubeExtraY = 3;
            //cubeExtraZ = 5;
            //translate([resetButtonX+0.01+1, resetButtonY-resetButtonRad-cubeExtraY/6-1.51, -resetButtonRad-cubeExtraZ/6-1])
            //{
            //    buttonCubeX = resetButtonLen-10.1;
            //    buttonCubeY = resetButtonRad+cubeExtraY+3;
            //    buttonCubeZ = resetButtonRad+cubeExtraY+2;
            //    cube([buttonCubeX, buttonCubeY, buttonCubeZ]);
                //translate([1-0.01, -2, 0])
                //cube([buttonCubeX-1+0.01, 2, buttonCubeZ]);
            //}

        }
        
        //translate([0, wemosLength/2+wemosYOffset-3, -0])
        //#wemosBoard();
        
        // Hole for MicroUSB
        translate([1, -10, 0])
        microUSBHole();
        
        //translate([-1, 0, 0])
        //scale([1.05, 1, 1])
        //resetButton();
        
        if (resetHole)
        {
            resetAccessHoleXY = 7;
            rotate([0, 0, -24])
            translate([0, cylBaseDia/2-1, 0])
            cube([resetAccessHoleXY, resetAccessHoleXY, 5], center=true);
        }
        
        showQRCode = true;
        if (showQRCode)
        {
            translate([0, 0, -cylBaseHeight/2-0.01])
            scale(0.5)
            linear_extrude(height=3)
            #import("qrcode.dxf", origin=[35, 35]);
        }
    }
    
    wemosMountH = 4;
    wemosMountW = 2;
    boostW = 3;
    boostH = 0.8;
    translate([0, cylBaseDia/2-wemosLength-8.25, -cylBaseHeight/2+cylThickness])
    union()
    {
        translate([wemosWidth/2, 0, 0])
        cube([wemosMountW, wemosLength-5, wemosMountH]);
        translate([-wemosWidth/2-wemosMountW, 0, 0])
        cube([wemosMountW, wemosLength-1, wemosMountH]);
        translate([-wemosWidth/2, 0, 0])
        cube([wemosWidth, wemosMountW, wemosMountH]);
        translate([-wemosWidth/2, 0, 0])
        cube([boostW, wemosLength-5, boostH]);
        translate([wemosWidth/2-boostW, 0, 0])
        cube([boostW, wemosLength-5, boostH]);
    }
}

module resetButton()
{
    splineXY = resetButtonRad/1.2;
    difference()
    {
        translate([resetButtonX, resetButtonY, 0])
        rotate([0, 90, 0])
        union()
        {
            // Circular button
            //cylinder(r=resetButtonRad, h=resetButtonLen);
            
            // Square button
            translate([-resetButtonRad, -resetButtonRad, 0])
            cube([resetButtonRad*2-0.01, resetButtonRad*2-0.01, resetButtonLen]);
            
            // Spline to keep button upright and locked
            translate([resetButtonRad-0.1, -splineXY/2, 0])
            cube([splineXY, splineXY, 5]);
        }
        
        translate([0, 0, 0.01])
        difference()
        {
            cylinder(r=cylBaseDia/2+cylThickness*2, h=cylBaseHeight, center=true);
            translate([0, 0, cylThickness])
            cylinder(r=cylBaseDia/2+0.4, h=cylBaseHeight, center=true);
        }
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
    pinSize = 8;
    pinsYOffset = 5.6;
    
    color("blue")
    difference()
    {
        cube([wemosWidth, wemosLength, wemosThickness], center=true);
    }
    
    translate([-wemosWidth/2, -wemosLength/2+pinsYOffset, wemosThickness/2])
    cube([pinHeaderWidth, pinHeaderWidth*pinSize, femalePinHeaderHeight]);
    
    translate([wemosWidth/2-pinHeaderWidth, -wemosLength/2+pinsYOffset, wemosThickness/2])
    cube([pinHeaderWidth, pinHeaderWidth*pinSize, femalePinHeaderHeight]);
}