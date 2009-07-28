/*Author: Chris Tralie
 *Project: Duke REU Fellowship 2009: Robotic navigation with RFID Waypoints
 *Purpose: To create a library in Java that can load PGM Images (the Java port of
 *pgm.cpp and pgm.h).  I also added the capability to generate special image buffers
 *for the map with the axes pre-drawn, or a buffer for heatmaps that has an alpha channel
 *so that the heatmaps can be transparently deposited on top of the occupancy grid*/

import java.io.*;
import java.util.*;
import java.awt.*;
import java.awt.image.*;

class PGMImage {
	public short[][] pixel;
	public int width, height, max;
	public String filename;
	public static Color transparent = new Color(0, true);
	
	//Translate the byte stream from the PGM image file into 
	//an array of greyscale values for this object
	//P5 Width Height Max <data...>
	public void interpretBytes(byte[] bytes) throws Exception {
		String str = new String(bytes);
		if (bytes[0] != 'P' || bytes[1] != '5')
			throw new Exception("Error: " + filename + " is not a valid PGM file");
		int i1 = 3;
		int i2 = str.indexOf(' ', i1);
		width = Integer.parseInt(str.substring(i1, i2));
		i1 = i2 + 1;
		i2 = str.indexOf(' ', i1);
		height = Integer.parseInt(str.substring(i1, i2));
		i1 = i2 + 1;
		if (!str.substring(i1, i1 + 3).equals("255")) {
			throw new Exception("Error: Max value is not 255; it is " + str.substring(i1, i1 + 3));
		}
		i1 += 3;
		while (str.charAt(i1) == ' ') i1++;
		pixel = new short[width][height];
		int imgindex = 0;
		for (int i = i1; i < bytes.length && imgindex < width * height; i++) {
			short val = bytes[i];
			if (val < 0) val += 256; //Two's complement
			pixel[imgindex % width][imgindex / width] = val;
			imgindex++;
		}
	}
	
	public PGMImage(String filename) throws Exception {
		File file = new File(filename);
		this.filename = filename;
		InputStream stream = new FileInputStream(file);
		long length = file.length();
		byte[] bytes = new byte[(int)length];
		int pos = 0;
		int numRead = 0;
		while (pos < bytes.length) {
			numRead = stream.read(bytes, pos, bytes.length - pos);
			pos += numRead;
			if (numRead < 0) break;
		}
		if (pos < bytes.length) {
			throw new IOException("Only read " + pos + " bytes out of " + bytes.length + " in file " + filename);
		}
		stream.close();
		interpretBytes(bytes);
	}
	
	//Use the HSL color model to get a shade of orange,
	//then convert it to RGB
	double getRGBComponent(double p, double q, double tc) {
		if (tc < 1.0 / 6.0)
			return p + ((q-p)*6*tc);
		if (tc < 0.5)
			return q;
		if (tc < 2.0 / 3.0)
			return p + ((q-p)*6*(2.0/3.0 - tc));
		return p;
	}
	
	Color getColorHSL(double paramh, double params, double l, int strength) {
		double h = paramh / 256.0;
		double s = params / 256.0;
		double p, q;
		if (l < 0.5)	
			q = l * (l + s);
		else			
			q = l + s - (l*s);
		p = 2*l - q;
		double tr = h + 1.0 / 3.0;
		double tg = h;
		double tb = h - 1.0 / 3.0;
		if (tr < 0)	tr = tr + 1;if (tr > 1) tr = tr - 1;
		if (tg < 0)	tg = tg + 1;if (tg > 1) tg = tg - 1;
		if (tb < 0)	tb = tb + 1;if (tb > 1) tb = tb - 1;
		int r = (int)(255.0 * getRGBComponent(p, q, tr));
		int g = (int)(255.0 * getRGBComponent(p, q, tg));
		int b = (int)(255.0 * getRGBComponent(p, q, tb));
		int rgb = ((r & 0xff) << 16) | ((g & 0xff) << 8) | (b & 0xff);
		return new Color(rgb);
	}
	
	//This is used primarily to return 
	public BufferedImage getBufferedImage() {
		BufferedImage image = new BufferedImage(width, height, BufferedImage.TYPE_INT_RGB);
		Graphics2D g = image.createGraphics();
		for (int i = 0; i < width; i++) {
			for (int j = 0; j < height; j++) {
				int val = pixel[i][j];
				Color color = new Color(val, val, val);
				g.setColor(color);
				g.fillRect(i, j, 1, 1);
			}
		}
		//Now draw the X and Y axes of the map
		g.setColor(Color.BLACK);
		g.drawLine(width/2, 0, width/2, height);
		g.drawLine(0, height/2, width, height/2);
		return image;
	}
	
	//Return a heatmap with an alpha channel.  Map the "grayscale" (tag reading intensity) values
	//to different brightnesses of orange, based on the functions above that convert the HSL model
	//to the RGB model.  Make the heatmap transparent when the tag was not seen, so that it can
	//be drawn over the occupancy grid.
	public BufferedImage getBufferedHeatmap() {
		BufferedImage image = new BufferedImage(width, height, BufferedImage.TYPE_INT_ARGB);
		Graphics2D g = image.createGraphics();
		//Draw everything transparent first, because the program appears to slow down
		//dramatically when it has to change the state from transparent to opaque
		g.setColor(transparent);
		g.fillRect(0, 0, width, height);
		for (int i = 0; i < width; i++) {
			for (int j = 0; j < height; j++) {
				int strength = pixel[i][j];
				double l = ((double)strength + 50.0) / 200.0;
				if (strength > 10) {
					g.setColor(getColorHSL(20.0, 240.0, l, strength));
					g.fillRect(i, j, 1, 1);
				}
			}
		}
		return image;
	}
}
