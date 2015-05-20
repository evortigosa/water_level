/*	Aluno: Evandro Scudeleti Ortigossa
	nUSP: 6793135	-   TCC parte I  */

/*#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"*/
#include <cv.h>
#include <highgui.h>
#include <iostream>
#include <cstdlib>
#include <math.h>

#define QTD_IMG 1
#define PROFUND 275

using namespace std;
using namespace cv;

Mat iminvert(Mat);
Mat imcontrast(Mat);
Mat grad_sobel(Mat);
Mat hough_transform(Mat);
int fatoracaoLU(float, float, float, float, float, float, float []);
int IsNumber(double);
float find_level(float, float, float, float, float, float, float);

int main(int argc, char **argv) {
    int xb, yb, xt, yt, wx, wy;
    float altura= 100;
    const char *in[QTD_IMG];

    if (argc< (QTD_IMG+ 5)) {
		cout << "./prog <param1> ... <param16>" << endl;
		cout << "\t param1 = imagem de entrada" << endl;    /* Parametros de entrada do programa */
		cout << "\t ..." << endl;                           /* 8 imagens para processamento */
		cout << "\t param8 = imagem de entrada" << endl;
		cout << "\t param9 = ponto x de observacao topo" << endl;  /* Coordenadas dos pontos de medicao */
		cout << "\t param10 = ponto y de observacao topo" << endl;  /* topo e base */
		cout << "\t param11 = ponto x de observacao base" << endl;
		cout << "\t param12 = ponto y de observacao base" << endl << endl;
		return -1;
	}

	for (int i= 0; i< QTD_IMG; i++) in[i]= argv[i+ 1];

	xt= atoi(argv[QTD_IMG+ 1]);
	yt= atoi(argv[QTD_IMG+ 2]);
	xb= atoi(argv[QTD_IMG+ 3]);
	yb= atoi(argv[QTD_IMG+ 4]);
	wx= atoi(argv[QTD_IMG+ 5]);
	wy= atoi(argv[QTD_IMG+ 6]);

    Mat in_atual, in_prox;
    Mat out_im;

    cout << endl;
    cout << "Altura da referencia: " << altura << " centimetros" << endl;
    cout << "Altura da parede: " << PROFUND << " centimetros" << endl << endl;

    for (int k= 0; k< QTD_IMG; k++) {
        in_atual= imread(in[k], CV_LOAD_IMAGE_GRAYSCALE);

        //in_atual= iminvert(in_atual);

        in_atual= imcontrast(in_atual);

        in_prox= grad_sobel(in_atual); /* Operador de Sobel para deteccao de contornos */

        threshold(in_prox, in_prox, 25, 255, CV_THRESH_BINARY);

        //out_im= hough_transform(out_im);    /* Aplica a Probabilistic Hough Line Transform */

        float m= (float)(yt- yb)/ (xt- xb);
        float a= -m;
        float b= 1;
        float c= (m* xb)- yb;

        cvtColor(in_prox, out_im, CV_GRAY2BGR);

        vector<Vec4i> lines;
        HoughLinesP(in_prox, lines, 1, CV_PI/180, 80, 80, 10);

        vector<Point> points;
        vector<int> points_i;

        for(size_t i= 0; i< lines.size(); i++) {

            float x1= lines[i][0];
            float y1= lines[i][1];

            float x2= lines[i][2];
            float y2= lines[i][3];

            //line(out_im, Point(lines[i][0], lines[i][1]),   // Point(col, row)
              //  Point(lines[i][2], lines[i][3]), Scalar(0, 0, 255), 20, 8);

            float m_aux= (y2- y1)/ (x2- x1);
            float a_aux= -m_aux;
            float b_aux= 1;
            float c_aux= (m_aux* x1)- y1;

            float resp[2];

            int pos= fatoracaoLU(a, b, (-c), a_aux, b_aux, (-c_aux), resp);

            if (pos!= 0) {      // row, col
                if (resp[1]> 0 && resp[0]> 0 &&
                    resp[1]>= yt && resp[0]>= xt &&
                    resp[1]<= wy && resp[0]<= wx &&
                    resp[1]>= y1 && resp[0]>= x1 &&
                    resp[1]<= y2 && resp[0]<= x2) {

                    points.push_back(Point(resp[0], resp[1]));
                    points_i.push_back(i);

                    //printf("%f %f\n\n\n", resp[0], resp[1]);

                    //line(out_im, Point(lines[i][0], lines[i][1]),   // Point(col, row)
                      //   Point(lines[i][2], lines[i][3]), Scalar(0, 255, 0), 20, 8);
                }
            }
        }
        int min_row= in_prox.rows;
        int row, col= 0, rc_i= 0;

        for(size_t j= 0; j< points.size(); j++) {
            Point p= points[j];
            row= p.y;

            int p_i= points_i[j];

            //printf("%d %d\n\n", col, row);

            if (min_row> row) {
                min_row= row;
                col= p.x;
                rc_i= p_i;
            }
        }
        row= min_row;

        //printf("%d %d\n\n", col, row);

        float nivel= find_level(altura, xb, yb, xt, yt, row, col);

        cout << "Nivel da agua na imagem " << k+ 1 << ": " << nivel << " centimetros" << endl << endl;

        line(out_im, Point(xb, yb), Point(xt, yt), Scalar(255, 0, 0), 20, 8);

        line(out_im, Point(lines[rc_i][0], lines[rc_i][1]),   // Point(col, row)
           Point(lines[rc_i][2], lines[rc_i][3]), Scalar(0, 255, 0), 1, 8);

        //line(out_im, Point(col, row), Point(col+1, row+1), Scalar(0, 0, 255), 20, 8);

        imwrite("imagem.jpg", out_im);

        namedWindow("K atual", CV_WINDOW_AUTOSIZE);	// Create a window for display.
        imshow("K atual", out_im);	    // Show our image inside it.
        waitKey(0);
    }

return 0;

}

Mat iminvert(Mat f) {
    int nivel;

    Mat new_image(f.size(), CV_8U, 1);

    for (int i= 0; i< f.rows; i++) {
        for (int j= 0; j< f.cols; j++) {

            nivel= (int)f.at<uchar>(i, j);  // Funcao de transformacao dos niveis de intensidade: negativo.
            nivel= 255- nivel;

			new_image.at<uchar>(i, j)= saturate_cast<uchar>(nivel);
		}
	}

	return new_image; // Retorna a imagem transformada de saida.
}

Mat imcontrast(Mat f) {
    int maxF= 0, minF= 255, nivel;

    Mat new_image(f.size(), CV_8U, 1);

    for (int i= 0; i< f.rows; i++) {
        for (int j= 0; j< f.cols; j++) {
            nivel= (int)f.at<uchar>(i, j);

            if (nivel> maxF) maxF= nivel;     // Armazena o valor maximo de intensidade encontrado na imagem.
            if (nivel< minF) minF= nivel;     // Armazena o valor minimo de intensidade encontrado na imagem.

        }
    }

    for (int k= 0; k< f.rows; k++) {
        for (int l= 0; l< f.cols; l++) {

            nivel= (int)f.at<uchar>(k, l);
            nivel= ((nivel- minF)* (255/ (maxF- minF)));    // Funcao de transformacao dos niveis de intensidade: contraste.

			new_image.at<uchar>(k, l)= saturate_cast<uchar>(nivel);
		}
	}

	return new_image;   // Retorna a imagem transformada de saida.
}

Mat grad_sobel(Mat im) {
    int scale= 1, delta= 0, ddepth= CV_16S;
    Mat src_gray, grad;

    GaussianBlur(im, src_gray, Size(15, 15), 0, 0, BORDER_DEFAULT); /* Borra imagem para evitar falsos positivos. */

    // Generate grad_x and grad_y
    Mat grad_x, grad_y;
    Mat abs_grad_x, abs_grad_y;

    // Gradient X
    Sobel(src_gray, grad_x, ddepth, 1, 0, 3, scale, delta, BORDER_DEFAULT);
    convertScaleAbs(grad_x, abs_grad_x);

    // Gradient Y
    Sobel( src_gray, grad_y, ddepth, 0, 1, 3, scale, delta, BORDER_DEFAULT);
    convertScaleAbs(grad_y, abs_grad_y);

    // Total Gradient (approximate)
    addWeighted(abs_grad_x, 0.5, abs_grad_y, 0.5, 0, grad);

    return grad;
}

Mat hough_transform(Mat im) {
    Mat src, color_src;

    src= 1* im;

    cvtColor(src, color_src, CV_GRAY2BGR);

    vector<Vec4i> lines;
    HoughLinesP(src, lines, 1, CV_PI/180, 80, 80, 10);

    for(size_t i= 0; i< lines.size(); i++) {
        line(color_src, Point(lines[i][0], lines[i][1]),
             Point(lines[i][2], lines[i][3]), Scalar(0, 0, 255), 3, 8);
    }

    return color_src;
}

int fatoracaoLU(float a1, float b1, float c1, float a2, float b2, float c2, float resp[2]) {
    int n= 2, i, j, k;
    double A[n][n], b[n], L[n][n], U[n][n], y[n], x[n], auxiliar;

    A[0][0]= a1;
    A[0][1]= b1;
    A[1][0]= a2;
    A[1][1]= b2;

    for ((i= 0); (i< n); (i++)) {
        for ((j= 0); (j< n); (j++)) {
            L[i][j]= 0;
            U[i][j]= 0;
        }
        y[i]= 0;
        x[i]= 0;
    }

    b[0]= c1;
    b[1]= c2;

    for ((i= 0); (i< n); (i++)) {
        L[i][0]= A[i][0]/ A[0][0];          /* Definição da primeira coluna e diagonal principal de L */
        L[i][i]= 1;

        U[0][i]= A[0][i];                   /* Definição da primeira linha de U */
    }

    for ((i= 1); (i< n); (i++)) {           /* Decomposição LU */
        for ((j= 1); (j< n); (j++)) {
            auxiliar= 0;

            if (i<= j) {
                for ((k= 0); (k< i); (k++)) auxiliar += (L[i][k]* U[k][j]);

                U[i][j]= A[i][j]- auxiliar;
            }
            else if (i> j) {
                for ((k= 0); (k< j); (k++)) auxiliar += (L[i][k]* U[k][j]);

                L[i][j]= (A[i][j]- auxiliar)/ U[j][j];
            }
        }
    }

    y[0]= b[0];

    for ((i= 1); (i< n); (i++)) {           /* Ly = b */
        auxiliar= 0;

        for ((j= 0); (j< i); (j++)) auxiliar += (L[i][j]* y[j]);

        y[i]= b[i]- auxiliar;
    }

    x[n- 1]= y[n- 1]/ U[n- 1][n- 1];

    for ((i= (n- 2)); (i>= 0); (i--)) {     /* Ux = y */
        auxiliar= 0;

        for ((j= (n- 1)); (j> i); (j--)) auxiliar += (U[i][j]* x[j]);

        x[i]= (y[i]- auxiliar)/ U[i][i];
    }

    for ((i= 0); (i< n); (i++)) {
        if (!IsNumber(x[i])) {              /* Testa se foi possível resolver o sistema */
            return 0;
        }

        resp[i]= x[i];
    }
    return 1;
}

int IsNumber(double x) {
    return (x== x);         /* Retorna falso se x é um valor nao numerico  */
}

float find_level(float altura, float xb, float yb, float xt, float yt, float row, float col) {

    if (row>= yt) {
        float h= fabs(row- yt);
        float b= fabs(col- xt);
        float hip= sqrt(pow(h, 2)+ pow(b, 2));

        float h_linha= fabs(row- yb);
        float b_linha= fabs(col- xb);
        float hip_linha= sqrt(pow(h_linha, 2)+ pow(b_linha, 2));

        float x= hip- hip_linha;
        float y= (hip* altura)/ x;

        float nivel= PROFUND- y;

        //printf("\n\n%f %f %f \n\n%f %f \n\n%f %f \n\n%f %f\n\n", nivel, y, x, hip, hip_linha, h, b, h_linha, b_linha);

        //printf("\n\n%f %f %f %f %f %f\n\n", xt, yt, xb, yb, col, row);

        return nivel;
    }
    return 0;
}
