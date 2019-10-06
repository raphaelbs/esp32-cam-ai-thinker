/**
 * This file builds the AWS s3 POST request in the format of a form data html page.
 * 
 * Once you fill your data in "Input data" section, execute this file using:
 * node html-generator
 * 
 * It will generate a "upload-to-s3.html" file that you can use to validate if your
 * input data is correct and if you have proper write access to s3 bucket. If you do
 * have proper access, the file should be uploaded to the bucket and the request
 * should return a 204 code.
 */

const fs = require('fs');
const { hmacSHA256, hmacSHA256Hex } = require('./hash');

// Input data
const ACCESS_KEY_ID = '[your access key id]';
const SECRET_ACCESS_KEY = '[your secret access key]';
const DATE = '[some date in yyyymmdd format]';
const AWS_REGION = 'us-east-1';
const BUCKET_NAME = '[bucket name]';
const POLICY_EXPIRATION_DATE = '2021-01-01T12:00:00.000Z';
const FILENAME = '${filename}';

const AMZ_CREDENTIAL = `${ACCESS_KEY_ID}/${DATE}/${AWS_REGION}/s3/aws4_request`;
const AMZ_DATE = `${DATE}T000000Z`;

// You may change the policy to include or remove fields
const POLICY = {
  expiration: POLICY_EXPIRATION_DATE,
  conditions: [
    // More info at https://docs.aws.amazon.com/pt_br/AmazonS3/latest/API/sigv4-HTTPPOSTConstructPolicy.html
    {bucket: BUCKET_NAME},
    ['starts-with', '$key', ''],
    ['content-length-range', 0, 1024 * 1024 ],
    {'x-amz-credential': AMZ_CREDENTIAL},
    {'x-amz-algorithm': 'AWS4-HMAC-SHA256'},
    {'x-amz-date': AMZ_DATE }
  ]
};

// Signature generation
const POLICY_BASE64 = Buffer.from(JSON.stringify(POLICY)).toString('base64');
let signingKey = hmacSHA256('AWS4' + SECRET_ACCESS_KEY, DATE);
signingKey = hmacSHA256(signingKey, AWS_REGION);
signingKey = hmacSHA256(signingKey, 's3');
signingKey = hmacSHA256(signingKey, 'aws4_request');
const AMZ_SIGNATURE = hmacSHA256Hex(signingKey, POLICY_BASE64);

// Output data
const body = {
  key: FILENAME,
  'x-amz-credential': AMZ_CREDENTIAL,
  'x-amz-algorithm': 'AWS4-HMAC-SHA256',
  'x-amz-date': AMZ_DATE,
  Policy: POLICY_BASE64,
  'x-amz-signature': AMZ_SIGNATURE,
  file: '[path to file]', // Note that the file should be the last field
};
console.log(JSON.stringify(body, null, 2)); // Those are the data you need

// Use this file to upload a file and test whether your data is correct.
fs.writeFileSync('./upload-to-s3.html', 
`<html>
  <head>
    <meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />
  </head>
  <body>

  <form action="http://${BUCKET_NAME}.s3.amazonaws.com/" method="post" enctype="multipart/form-data">
    Key to upload: 
    <input type="input"  name="key" value="${FILENAME}" /> <br />
    <input type="hidden" name="x-amz-credential" value="${AMZ_CREDENTIAL}" />
    <input type="hidden" name="x-amz-algorithm" value="AWS4-HMAC-SHA256" />
    <input type="hidden" name="x-amz-date" value="${AMZ_DATE}" />
    <input type="hidden" name="policy" value="${POLICY_BASE64}" />
    <input type="hidden" name="x-amz-signature" value="${AMZ_SIGNATURE}" />
    File: 
    <input type="file"   name="file" /> <br /><br />
    <!-- The elements after this will be ignored -->
    <input type="submit" name="submit" value="Upload to Amazon S3" />
  </form>
</html>`);
