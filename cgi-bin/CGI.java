class addMul_Script{
    public static void main (String[] args) {
        double firstNum;
        double secondNum;
        String result;
        String [] lines = args[0].split("\n");
        if (lines[0].substring(0,3).equals("GET")) {
            firstNum = Double.parseDouble(lines[0].split("\\?")[1].split("&")[0].split("=")[1]);
            secondNum = Double.parseDouble(lines[0].split("\\?")[1].split("&")[1].split("=")[1]);
            result = Double.toString(firstNum + secondNum);

            System.out.println( "HTTP/1.0 200 OK\n" +
                    "Content-type: text/html\n\n" +
                    "<html>" +
                    "<body>" +

                    "<h1>MatzzRokzz</h1>" +
                    "<img src=\"../images/Math.gif\">" +


                    "<H1> Please use the following form to add two numbers</H1>" +
                    "<FORM ACTION=http://localhost:23456/cgi-bin/addMul_Script.class METHOD=GET>" +
                    "Num1: <INPUT NAME=FirstNum> <br>" +
                    "Num2: <INPUT NAME=SecondNum><br>" +
                    "<INPUT TYPE=SUBMIT NAME=GO><br>" +
                    "<H2> Result equal = " + result + "</H2>" +
                    "</FORM>" +

                    "<H1> Please use the following form to multiply two numbers</H1>" +
                    "<FORM ACTION=http://localhost:23456/cgi-bin/addMul_Script.class METHOD=POST>" +
                    "Num1: <INPUT NAME=FirstNum> <br>" +
                    "Num2: <INPUT NAME=SecondNum><br>" +
                    "<INPUT TYPE=SUBMIT NAME=GO><br>" +
//                    "<H2> Result equal = " + result + "</H2>" +
                    "</FORM>" +

                    "<H2> Thank you</H2>" +
                    "</body>" +
                    "</html>");

        } else{
            firstNum = Double.parseDouble(lines[lines.length-1].split("&")[0].split("=")[1]);
            secondNum = Double.parseDouble(lines[lines.length-1].split("&")[1].split("=")[1]);
            result = Double.toString(firstNum * secondNum);

            System.out.println( "HTTP/1.0 200 OK\n" +
                    "Content-type: text/html\n\n" +
                    "<html>" +
                    "<body>" +

                    "<h1>MatzzRokzz</h1>" +
                    "<img src=\"../images/Math.gif\">" +


                    "<H1> Please use the following form to add two numbers</H1>" +
                    "<FORM ACTION=http://localhost:23456/cgi-bin/addMul_Script.class METHOD=GET>" +
                    "Num1: <INPUT NAME=FirstNum> <br>" +
                    "Num2: <INPUT NAME=SecondNum><br>" +
                    "<INPUT TYPE=SUBMIT NAME=GO><br>" +
//                    "<H2> Result equal = " + result + "</H2>" +
                    "</FORM>" +

                    "<H1> Please use the following form to multiply two numbers</H1>" +
                    "<FORM ACTION=http://localhost:23456/cgi-bin/addMul_Script.class METHOD=POST>" +
                    "Num1: <INPUT NAME=FirstNum> <br>" +
                    "Num2: <INPUT NAME=SecondNum><br>" +
                    "<INPUT TYPE=SUBMIT NAME=GO><br>" +
                    "<H2> Result equal = " + result + "</H2>" +
                    "</FORM>" +

                    "<H2> Thank you</H2>" +
                    "</body>" +
                    "</html>");

        }

    }
}
